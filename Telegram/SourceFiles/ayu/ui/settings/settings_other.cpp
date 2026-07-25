// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_other.h"

#include "api/api_premium.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_ayu_utils.h"
#include "ayu/ui/settings/settings_main.h"
#include "ayu/ui/utils/ayu_profile_values.h"
#include "apiwrap.h"
#include "boxes/abstract_box.h"
#include "boxes/star_gift_box.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "data/data_changes.h"
#include "data/data_session.h"
#include "data/data_star_gift.h"
#include "data/data_user.h"
#include "info/peer_gifts/info_peer_gifts_common.h"
#include "lang/lang_text_entity.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "ui/boxes/confirm_box.h"
#include "ui/image/image.h"
#include "ui/layers/generic_box.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"
#include "window/themes/window_theme.h"
#include "window/window_session_controller.h"

#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QPointer>
#include <QRegularExpression>

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

void BuildCrashReporting(SectionBuilder &builder, AyuSectionBuilder &ayu) {
#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	builder.addSubsectionTitle(tr::ayu_DiagnosticsHeader());

	ayu.addSettingToggle({
		.id = u"ayu/crashReporting"_q,
		.altIds = { u"ayu/crashlytics"_q },
		.title = tr::ayu_CrashReporting(),
		.getter = &AyuSettings::crashReporting,
		.setter = &AyuSettings::setCrashReporting,
		.icon = { &st::menuIconReport },
	});
	builder.addSkip();
	builder.addDividerText(tr::ayu_CrashReportingDescription());
#endif
}

enum class LocalGiftTarget {
	Featured,
	Pinned,
};

struct LocalGiftCollection {
	QString title;
	QString slug;
};

struct LocalGiftBrowserState {
	rpl::variable<Ui::GiftsDescriptor> gifts;
	std::vector<Info::PeerGifts::GiftDescriptor> items;
	std::vector<LocalGiftCollection> collections;
	std::unique_ptr<Api::PremiumGiftCodeOptions> catalog;
	rpl::lifetime catalogLifetime;
	int nextNumber = 1;
	bool loading = false;
	bool exhausted = false;
};

constexpr auto kLocalGiftBrowserPageSize = 12;

[[nodiscard]] std::vector<LocalGiftCollection> LocalGiftCollections() {
	return {
		{ u"Scared Cat"_q, u"ScaredCat"_q },
		{ u"Plush Pepe"_q, u"PlushPepe"_q },
		{ u"Toy Bear"_q, u"ToyBear"_q },
		{ u"Chill Flame"_q, u"ChillFlame"_q },
		{ u"Precious Peach"_q, u"PreciousPeach"_q },
		{ u"Heart Locket"_q, u"HeartLocket"_q },
		{ u"Diamond Ring"_q, u"DiamondRing"_q },
		{ u"Astral Shard"_q, u"AstralShard"_q },
	};
}

[[nodiscard]] LocalGiftCollection LocalGiftCollectionFromTitle(
		QString title) {
	title = title.trimmed();
	auto slug = title;
	slug.remove(QRegularExpression(u"[^A-Za-z0-9]"_q));
	return slug.isEmpty()
		? LocalGiftCollection()
		: LocalGiftCollection{ std::move(title), std::move(slug) };
}

[[nodiscard]] std::vector<QString> LocalPinnedGiftReferences() {
	auto result = std::vector<QString>();
	const auto normalized = Ayu::NormalizeLocalProfileGifts(
		AyuSettings::getInstance().localProfileGifts());
	for (const auto &part : normalized.split(',', Qt::SkipEmptyParts)) {
		const auto reference = part.trimmed();
		if (!reference.isEmpty()) {
			result.push_back(reference);
		}
	}
	return result;
}

[[nodiscard]] QString SerializeLocalGiftReferences(
		const std::vector<QString> &references) {
	auto result = QStringList();
	result.reserve(references.size());
	for (const auto &reference : references) {
		result.push_back(reference);
	}
	return result.join(u", "_q);
}

[[nodiscard]] QString LocalGiftReferenceLabel(QString reference) {
	reference = Ayu::NormalizeLocalProfileGift(std::move(reference));
	static const auto Slug = QRegularExpression(u"^(.+)-([0-9]+)$"_q);
	const auto match = Slug.match(reference);
	if (match.hasMatch()) {
		auto title = match.captured(1);
		title.replace(
			QRegularExpression(u"([a-z0-9])([A-Z])"_q),
			u"\\1 \\2"_q);
		return title + u" #"_q + match.captured(2);
	}
	return (reference.size() > 38)
		? (reference.left(24) + u"..."_q + reference.right(8))
		: reference;
}

void ShowLocalGiftCollectionPicker(
	not_null<Window::SessionController*> controller,
	LocalGiftTarget target);

[[nodiscard]] bool StoreLocalGift(
		not_null<Window::SessionController*> controller,
		LocalGiftTarget target,
		const Data::StarGift &gift) {
	if (!gift.unique || gift.unique->slug.isEmpty()) {
		controller->showToast(
			tr::ayu_LocalProfileGiftPickerInvalid(tr::now));
		return false;
	}
	auto &settings = AyuSettings::getInstance();
	const auto slug = gift.unique->slug;
	if (target == LocalGiftTarget::Featured) {
		settings.setLocalProfileGift(slug);
	} else {
		auto references = LocalPinnedGiftReferences();
		const auto duplicate = ranges::any_of(
			references,
			[&](const auto &entry) {
				return entry.compare(slug, Qt::CaseInsensitive) == 0;
			});
		if (!duplicate && references.size() >= 6) {
			controller->showToast(
				tr::ayu_LocalProfileGiftPickerLimit(tr::now));
			return false;
		}
		if (!duplicate) {
			references.push_back(slug);
			settings.setLocalProfileGifts(
				SerializeLocalGiftReferences(references));
		}
	}
	settings.setLocalProfileGiftEnabled(true);
	controller->showToast(tr::ayu_LocalProfileGiftPickerSaved(
		tr::now,
		lt_gift,
		Data::UniqueGiftName(*gift.unique)));
	return true;
}

[[nodiscard]] QString ResolveLocalGiftReference(
		QString reference,
		const LocalGiftCollection &collection) {
	reference = reference.trimmed();
	while (reference.startsWith('#')) {
		reference = reference.mid(1).trimmed();
	}
	auto onlyDigits = !reference.isEmpty();
	for (const auto ch : reference) {
		if (!ch.isDigit()) {
			onlyDigits = false;
			break;
		}
	}
	if (onlyDigits && !collection.slug.isEmpty()) {
		reference = collection.slug + '-' + reference;
	}
	return Ayu::NormalizeLocalProfileGift(std::move(reference));
}

void LoadLocalGiftBrowserPage(
		not_null<Window::SessionController*> controller,
		LocalGiftCollection collection,
		QPointer<Ui::GenericBox> box,
		not_null<LocalGiftBrowserState*> state) {
	if (state->loading || state->exhausted || !box) {
		return;
	}
	state->loading = true;
	auto references = std::vector<QString>();
	references.reserve(kLocalGiftBrowserPageSize);
	for (auto i = 0; i != kLocalGiftBrowserPageSize; ++i) {
		references.push_back(
			collection.slug + '-' + QString::number(state->nextNumber++));
	}
	Ayu::ResolveLocalProfileGifts(
		&controller->session(),
		std::move(references),
		[=](std::vector<std::shared_ptr<Data::StarGift>> gifts) {
			if (!box) {
				return;
			}
			state->loading = false;
			auto resolved = 0;
			for (const auto &gift : gifts) {
				if (gift && gift->unique) {
					++resolved;
					state->items.emplace_back(
						Info::PeerGifts::GiftTypeStars{ .info = *gift });
				}
			}
			state->exhausted = (resolved == 0);
			state->gifts = Ui::GiftsDescriptor{ .list = state->items };
		});
}

void LoadLocalGiftCollectionFallback(
		not_null<Window::SessionController*> controller,
		QPointer<Ui::GenericBox> box,
		not_null<LocalGiftBrowserState*> state) {
	if (!box || !state->items.empty()) {
		return;
	}
	state->collections = LocalGiftCollections();
	state->items.clear();
	auto references = std::vector<QString>();
	references.reserve(state->collections.size());
	for (const auto &collection : state->collections) {
		references.push_back(collection.slug + u"-1"_q);
	}
	Ayu::ResolveLocalProfileGifts(
		&controller->session(),
		std::move(references),
		[=](std::vector<std::shared_ptr<Data::StarGift>> gifts) {
			if (!box || !state->items.empty()) {
				return;
			}
			for (const auto &gift : gifts) {
				if (gift && gift->unique) {
					state->items.emplace_back(
						Info::PeerGifts::GiftTypeStars{ .info = *gift });
				}
			}
			state->gifts = Ui::GiftsDescriptor{ .list = state->items };
		});
}

[[nodiscard]] bool ApplyLocalGiftCatalog(
		QPointer<Ui::GenericBox> box,
		not_null<LocalGiftBrowserState*> state) {
	if (!box || !state->catalog) {
		return false;
	}
	state->collections.clear();
	state->items.clear();
	for (const auto &gift : state->catalog->starGifts()) {
		const auto collection = LocalGiftCollectionFromTitle(
			gift.resellTitle);
		if (collection.slug.isEmpty()) {
			continue;
		}
		const auto duplicate = ranges::any_of(
			state->collections,
			[&](const auto &entry) {
				return entry.slug.compare(
					collection.slug,
					Qt::CaseInsensitive) == 0;
			});
		if (duplicate) {
			continue;
		}
		state->collections.push_back(collection);
		state->items.emplace_back(
			Info::PeerGifts::GiftTypeStars{ .info = gift });
	}
	if (state->items.empty()) {
		return false;
	}
	state->gifts = Ui::GiftsDescriptor{ .list = state->items };
	return true;
}

void LoadLocalGiftCollections(
		not_null<Window::SessionController*> controller,
		QPointer<Ui::GenericBox> box,
		not_null<LocalGiftBrowserState*> state) {
	state->catalog = std::make_unique<Api::PremiumGiftCodeOptions>(
		controller->session().user());
	state->catalogLifetime = state->catalog->requestStarGifts(
	) | rpl::on_error_done([=](const QString &) {
		LoadLocalGiftCollectionFallback(controller, box, state);
	}, [=] {
		if (!ApplyLocalGiftCatalog(box, state)) {
			LoadLocalGiftCollectionFallback(controller, box, state);
		}
	});
}

void ShowLocalGiftCollectionBrowser(
		not_null<Window::SessionController*> controller,
		LocalGiftTarget target,
		LocalGiftCollection collection) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(rpl::single(collection.title));
		const auto content = box->verticalLayout();
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::ayu_LocalProfileGiftPickerNumber()),
			st::boxRowPadding);
		field->setMaxLength(Ayu::kMaxLocalProfileGiftReferenceLength);
		AddSubsectionTitle(
			content,
			tr::ayu_LocalProfileGiftPickerItems());

		const auto state = box->lifetime().make_state<LocalGiftBrowserState>();
		const auto weak = QPointer<Ui::GenericBox>(box.get());
		content->add(Ui::MakeGiftsList({
			.window = controller,
			.peer = controller->session().user(),
			.gifts = state->gifts.value(),
			.loadMore = [=] {
				LoadLocalGiftBrowserPage(
					controller,
					collection,
					weak,
					state);
			},
			.handler = [=](Info::PeerGifts::GiftDescriptor descriptor) {
				const auto gift = std::get_if<
					Info::PeerGifts::GiftTypeStars>(&descriptor);
				if (gift && StoreLocalGift(controller, target, gift->info)) {
					if (weak) {
						weak->closeBox();
					}
				}
			},
		}));

		const auto saveReference = [=] {
			const auto reference = ResolveLocalGiftReference(
				field->getLastText(),
				collection);
			if (reference.isEmpty()) {
				controller->showToast(
					tr::ayu_LocalProfileGiftPickerInvalid(tr::now));
				return;
			}
			controller->showToast(
				tr::ayu_LocalProfileGiftPickerResolving(tr::now));
			Ayu::ResolveLocalProfileGift(
				&controller->session(),
				reference,
				[=](std::shared_ptr<Data::StarGift> gift) {
					if (!weak) {
						return;
					}
					if (gift && StoreLocalGift(controller, target, *gift)) {
						weak->closeBox();
					} else if (!gift) {
						controller->showToast(
							tr::ayu_LocalProfileGiftPickerInvalid(tr::now));
					}
				});
		};
		field->submits(
		) | rpl::on_next(saveReference, field->lifetime());
		box->addButton(
			target == LocalGiftTarget::Featured
				? tr::ayu_LocalProfileGiftPickerUseFeatured()
				: tr::ayu_LocalProfileGiftPickerAddPinned(),
			saveReference);
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
		LoadLocalGiftBrowserPage(controller, collection, weak, state);
	}));
}

void ShowLocalGiftReferenceBox(
		not_null<Window::SessionController*> controller,
		LocalGiftTarget target,
		LocalGiftCollection collection) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(collection.title.isEmpty()
			? tr::ayu_LocalProfileGiftPickerPasteTitle()
			: rpl::single(collection.title));
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::ayu_LocalProfileGiftPickerNumber()),
			st::boxRowPadding);
		field->setMaxLength(Ayu::kMaxLocalProfileGiftReferenceLength);
		box->setFocusCallback([=] {
			field->setFocusFast();
		});
		AddDividerText(
			box->verticalLayout(),
			collection.slug.isEmpty()
				? tr::ayu_LocalProfileGiftPickerPasteAbout()
				: tr::ayu_LocalProfileGiftPickerNumberAbout());

		const auto save = [=] {
			auto reference = ResolveLocalGiftReference(
				field->getLastText(),
				collection);
			if (reference.isEmpty()) {
				controller->showToast(
					tr::ayu_LocalProfileGiftPickerInvalid(tr::now));
				return;
			}

			controller->showToast(
				tr::ayu_LocalProfileGiftPickerResolving(tr::now));
			const auto weak = QPointer<Ui::GenericBox>(box.get());
			Ayu::ResolveLocalProfileGift(
				&controller->session(),
				std::move(reference),
				[=](std::shared_ptr<Data::StarGift> gift) {
					if (!weak) {
						return;
					}
					if (!gift) {
						controller->showToast(
							tr::ayu_LocalProfileGiftPickerInvalid(tr::now));
						return;
					}
					if (StoreLocalGift(controller, target, *gift)) {
						weak->closeBox();
					}
				});
		};
		field->submits(
		) | rpl::on_next(save, field->lifetime());
		box->addButton(
			target == LocalGiftTarget::Featured
				? tr::ayu_LocalProfileGiftPickerUseFeatured()
				: tr::ayu_LocalProfileGiftPickerAddPinned(),
			save);
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
}

void ShowLocalGiftCollectionPicker(
		not_null<Window::SessionController*> controller,
		LocalGiftTarget target) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_LocalProfileGiftPickerTitle());
		const auto content = box->verticalLayout();
		AddSubsectionTitle(
			content,
			tr::ayu_LocalProfileGiftPickerCollection());
		const auto state = box->lifetime().make_state<LocalGiftBrowserState>();
		const auto weak = QPointer<Ui::GenericBox>(box.get());
		content->add(Ui::MakeGiftsList({
			.window = controller,
			.peer = controller->session().user(),
			.gifts = state->gifts.value(),
			.handler = [=](Info::PeerGifts::GiftDescriptor descriptor) {
				const auto gift = std::get_if<
					Info::PeerGifts::GiftTypeStars>(&descriptor);
				if (!gift) {
					return;
				}
				for (const auto &collection : state->collections) {
					const auto titleMatches = !gift->info.resellTitle.isEmpty()
						&& (gift->info.resellTitle.compare(
							collection.title,
							Qt::CaseInsensitive) == 0);
					const auto slugMatches = gift->info.unique
						&& gift->info.unique->slug.startsWith(
							collection.slug + '-',
							Qt::CaseInsensitive);
					if (titleMatches || slugMatches) {
						ShowLocalGiftCollectionBrowser(
							controller,
							target,
							collection);
						if (weak) {
							weak->closeBox();
						}
						return;
					}
				}
			},
		}));
		AddSkip(content);
		AddDivider(content);
		AddSkip(content);
		const auto paste = AddButtonWithIcon(
			content,
			tr::ayu_LocalProfileGiftPickerPaste(),
			st::settingsButtonActive,
			{ &st::menuIconLink });
		paste->setClickedCallback([=] {
			ShowLocalGiftReferenceBox(controller, target, {});
			box->closeBox();
		});
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});

		LoadLocalGiftCollectionFallback(controller, weak, state);
		LoadLocalGiftCollections(controller, weak, state);
	}));
}

void ShowLocalPinnedGiftManager(
		not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::ayu_LocalProfileGiftPickerManagePinned());
		const auto content = box->verticalLayout();
		const auto add = AddButtonWithIcon(
			content,
			tr::ayu_LocalProfileGiftPickerAdd(),
			st::settingsButtonActive,
			{ &st::menuIconGiftPremium });
		add->setClickedCallback([=] {
			ShowLocalGiftCollectionPicker(controller, LocalGiftTarget::Pinned);
			box->closeBox();
		});

		const auto references = LocalPinnedGiftReferences();
		if (references.empty()) {
			AddDividerText(
				content,
				tr::ayu_LocalProfileGiftPickerPinnedEmpty());
		} else {
			AddSkip(content);
			AddSubsectionTitle(
				content,
				tr::ayu_LocalProfileGiftPickerCurrent());
			for (const auto &reference : references) {
				const auto name = std::make_shared<rpl::variable<QString>>(
					LocalGiftReferenceLabel(reference));
				const auto button = AddButtonWithIcon(
					content,
					name->value() | rpl::map([](QString value) {
						return tr::ayu_LocalProfileGiftPickerRemove(
							tr::now,
							lt_gift,
							std::move(value));
					}),
					st::settingsButton,
					{ &st::menuIconDelete });
				button->setClickedCallback([=] {
					auto kept = std::vector<QString>();
					for (const auto &entry : LocalPinnedGiftReferences()) {
						if (entry.compare(
								reference,
								Qt::CaseInsensitive) != 0) {
							kept.push_back(entry);
						}
					}
					AyuSettings::getInstance().setLocalProfileGifts(
						SerializeLocalGiftReferences(kept));
					button->hide();
				});
				Ayu::ResolveLocalProfileGift(
					&controller->session(),
					reference,
					[name](std::shared_ptr<Data::StarGift> gift) {
						if (gift && gift->unique) {
							*name = Data::UniqueGiftName(*gift->unique);
						}
					});
			}
			AddSkip(content);
			const auto clear = AddButtonWithIcon(
				content,
				tr::ayu_LocalProfileGiftPickerClearPinned(),
				st::settingsButton,
				{ &st::menuIconDelete });
			clear->setClickedCallback([=] {
				AyuSettings::getInstance().setLocalProfileGifts(QString());
				box->closeBox();
			});
		}
		AddDividerText(content, tr::ayu_LocalProfileGiftPickerLocalOnly());
		box->addButton(tr::lng_box_done(), [=] {
			box->closeBox();
		});
	}));
}

void ChooseLocalProfilePhoto(
		not_null<Window::SessionController*> controller) {
	const auto filter = u"Image files (*.png *.jpg *.jpeg *.webp *.heic);;"_q
		+ FileDialog::AllFilesFilter();
	FileDialog::GetOpenPath(
		Core::App().getFileDialogParent(),
		tr::ayu_LocalProfilePhotoChoose(tr::now),
		filter,
		[=](const FileDialog::OpenResult &result) {
			if (result.paths.isEmpty()) {
				return;
			}
			const auto path = result.paths.front();
			const auto image = Images::Read({ .path = path }).image;
			if (image.isNull()) {
				controller->showToast(
					tr::ayu_LocalProfilePhotoInvalid(tr::now));
				return;
			}
			AyuSettings::getInstance().setLocalProfilePhoto(path);
			Ayu::RefreshLocalProfilePresentation(&controller->session());
			controller->showToast(
				tr::ayu_LocalProfilePhotoSaved(tr::now));
		});
}

void ShowLocalProfileCloneBox(
		not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::ayu_LocalProfileCloneTitle());
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::ayu_LocalProfileCloneId(),
				AyuSettings::getInstance().localProfileCloneId()),
			st::boxRowPadding);
		field->setMaxLength(Ayu::kMaxLocalProfileIdLength);
		box->setFocusCallback([=] {
			field->setFocusFast();
		});
		AddDividerText(
			box->verticalLayout(),
			tr::ayu_LocalProfileCloneDescription());

		const auto save = [=] {
			const auto id = Ayu::NormalizeLocalProfileCloneId(
				field->getLastText());
			const auto value = id.toULongLong();
			const auto user = value
				? controller->session().data().userLoaded(UserId(value))
				: nullptr;
			if (!user || user->isSelf()) {
				controller->showToast(
					tr::ayu_LocalProfileCloneNotFound(tr::now));
				return;
			}
			AyuSettings::getInstance().setLocalProfileCloneId(id);
			controller->session().api().requestFullPeer(user);
			Ayu::RefreshLocalProfilePresentation(&controller->session());
			controller->showToast(tr::ayu_LocalProfileCloneSaved(
				tr::now,
				lt_user,
				user->name()));
			box->closeBox();
		};
		field->submits(
		) | rpl::on_next(save, field->lifetime());
		box->addButton(tr::ayu_LocalProfileCloneUse(), save);
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
}

void BuildLocalProfile(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_LocalProfileHeader());
	builder.addButton({
		.id = u"ayu/localProfileClone"_q,
		.title = tr::ayu_LocalProfileClone(),
		.icon = { &st::menuIconProfile },
		.onClick = [=] {
			ShowLocalProfileCloneBox(controller);
		},
	});
	builder.addButton({
		.id = u"ayu/localProfileCloneClear"_q,
		.title = tr::ayu_LocalProfileCloneClear(),
		.icon = { &st::menuIconDelete },
		.onClick = [=] {
			AyuSettings::getInstance().setLocalProfileCloneId(QString());
			Ayu::RefreshLocalProfilePresentation(&controller->session());
			controller->showToast(tr::lng_box_done(tr::now));
		},
	});
	builder.addDividerText(tr::ayu_LocalProfileCloneLocalOnly());
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_LocalProfileIdentityHeader());

	ayu.addSettingToggle({
		.id = u"ayu/localProfileNameEnabled"_q,
		.title = tr::ayu_LocalProfileNameEnabled(),
		.getter = &AyuSettings::localProfileNameEnabled,
		.setter = &AyuSettings::setLocalProfileNameEnabled,
		.icon = { &st::menuIconProfile },
	});
	builder.add([](const WidgetContext &wctx) {
		auto field = object_ptr<Ui::InputField>(
			wctx.container,
			st::settingsDeviceName,
			tr::ayu_LocalProfileName(),
			AyuSettings::getInstance().localProfileName());
		const auto raw = field.data();
		raw->setMaxLength(Ayu::kMaxLocalProfileNameLength);
		raw->changes(
		) | rpl::on_next([=] {
			AyuSettings::getInstance().setLocalProfileName(
				raw->getLastText());
		}, raw->lifetime());
		return SectionBuilder::WidgetToAdd{
			.widget = std::move(field),
			.margin = st::boxRowPadding - style::margins(
				st::settingsDeviceName.textMargins.left(),
				0,
				st::settingsDeviceName.textMargins.right(),
				0),
		};
	}, [] {
		return SearchEntry{
			.id = u"ayu/localProfileName"_q,
			.title = tr::ayu_LocalProfileName(tr::now),
			.icon = { &st::menuIconProfile },
		};
	});
	ayu.addSettingToggle({
		.id = u"ayu/localProfileIdEnabled"_q,
		.title = tr::ayu_LocalProfileIdEnabled(),
		.getter = &AyuSettings::localProfileIdEnabled,
		.setter = &AyuSettings::setLocalProfileIdEnabled,
		.icon = { &st::menuIconInfo },
	});
	builder.add([](const WidgetContext &wctx) {
		auto field = object_ptr<Ui::InputField>(
			wctx.container,
			st::settingsDeviceName,
			tr::ayu_LocalProfileId(),
			AyuSettings::getInstance().localProfileId());
		const auto raw = field.data();
		raw->setMaxLength(Ayu::kMaxLocalProfileIdLength);
		raw->changes(
		) | rpl::on_next([=] {
			AyuSettings::getInstance().setLocalProfileId(
				raw->getLastText());
		}, raw->lifetime());
		return SectionBuilder::WidgetToAdd{
			.widget = std::move(field),
			.margin = st::boxRowPadding - style::margins(
				st::settingsDeviceName.textMargins.left(),
				0,
				st::settingsDeviceName.textMargins.right(),
				0),
		};
	}, [] {
		return SearchEntry{
			.id = u"ayu/localProfileId"_q,
			.title = tr::ayu_LocalProfileId(tr::now),
			.icon = { &st::menuIconInfo },
		};
	});
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_LocalProfileContactHeader());
	ayu.addSettingToggle({
		.id = u"ayu/localProfileUsernameEnabled"_q,
		.title = tr::ayu_LocalProfileUsernameEnabled(),
		.getter = &AyuSettings::localProfileUsernameEnabled,
		.setter = &AyuSettings::setLocalProfileUsernameEnabled,
		.icon = { &st::menuIconLink },
	});
	builder.add([](const WidgetContext &wctx) {
		auto field = object_ptr<Ui::InputField>(
			wctx.container,
			st::settingsDeviceName,
			tr::ayu_LocalProfileUsername(),
			AyuSettings::getInstance().localProfileUsername());
		const auto raw = field.data();
		raw->setMaxLength(Ayu::kMaxLocalProfileUsernameLength);
		raw->changes(
		) | rpl::on_next([=] {
			AyuSettings::getInstance().setLocalProfileUsername(
				raw->getLastText());
		}, raw->lifetime());
		return SectionBuilder::WidgetToAdd{
			.widget = std::move(field),
			.margin = st::boxRowPadding - style::margins(
				st::settingsDeviceName.textMargins.left(),
				0,
				st::settingsDeviceName.textMargins.right(),
				0),
		};
	}, [] {
		return SearchEntry{
			.id = u"ayu/localProfileUsername"_q,
			.title = tr::ayu_LocalProfileUsername(tr::now),
			.icon = { &st::menuIconLink },
		};
	});
	builder.add([](const WidgetContext &wctx) {
		auto field = object_ptr<Ui::InputField>(
			wctx.container,
			st::settingsDeviceName,
			tr::ayu_LocalProfileUsernames(),
			AyuSettings::getInstance().localProfileUsernames());
		const auto raw = field.data();
		raw->setMaxLength(Ayu::kMaxLocalProfileUsernamesLength);
		raw->changes(
		) | rpl::on_next([=] {
			AyuSettings::getInstance().setLocalProfileUsernames(
				raw->getLastText());
		}, raw->lifetime());
		return SectionBuilder::WidgetToAdd{
			.widget = std::move(field),
			.margin = st::boxRowPadding - style::margins(
				st::settingsDeviceName.textMargins.left(),
				0,
				st::settingsDeviceName.textMargins.right(),
				0),
		};
	}, [] {
		return SearchEntry{
			.id = u"ayu/localProfileUsernames"_q,
			.title = tr::ayu_LocalProfileUsernames(tr::now),
			.icon = { &st::menuIconLink },
		};
	});
	ayu.addSettingToggle({
		.id = u"ayu/localProfilePhoneEnabled"_q,
		.title = tr::ayu_LocalProfilePhoneEnabled(),
		.getter = &AyuSettings::localProfilePhoneEnabled,
		.setter = &AyuSettings::setLocalProfilePhoneEnabled,
		.icon = { &st::menuIconPhone },
	});
	builder.add([](const WidgetContext &wctx) {
		auto field = object_ptr<Ui::InputField>(
			wctx.container,
			st::settingsDeviceName,
			tr::ayu_LocalProfilePhone(),
			AyuSettings::getInstance().localProfilePhone());
		const auto raw = field.data();
		raw->setMaxLength(Ayu::kMaxLocalProfilePhoneLength);
		raw->changes(
		) | rpl::on_next([=] {
			AyuSettings::getInstance().setLocalProfilePhone(
				raw->getLastText());
		}, raw->lifetime());
		return SectionBuilder::WidgetToAdd{
			.widget = std::move(field),
			.margin = st::boxRowPadding - style::margins(
				st::settingsDeviceName.textMargins.left(),
				0,
				st::settingsDeviceName.textMargins.right(),
				0),
		};
	}, [] {
		return SearchEntry{
			.id = u"ayu/localProfilePhone"_q,
			.title = tr::ayu_LocalProfilePhone(tr::now),
			.icon = { &st::menuIconPhone },
		};
	});
	ayu.addSettingToggle({
		.id = u"ayu/localProfileAboutEnabled"_q,
		.title = tr::ayu_LocalProfileAboutEnabled(),
		.getter = &AyuSettings::localProfileAboutEnabled,
		.setter = &AyuSettings::setLocalProfileAboutEnabled,
		.icon = { &st::menuIconInfo },
	});
	builder.add([](const WidgetContext &wctx) {
		auto field = object_ptr<Ui::InputField>(
			wctx.container,
			st::settingsDeviceName,
			tr::ayu_LocalProfileAbout(),
			AyuSettings::getInstance().localProfileAbout());
		const auto raw = field.data();
		raw->setMaxLength(Ayu::kMaxLocalProfileAboutLength);
		raw->changes(
		) | rpl::on_next([=] {
			AyuSettings::getInstance().setLocalProfileAbout(
				raw->getLastText());
		}, raw->lifetime());
		return SectionBuilder::WidgetToAdd{
			.widget = std::move(field),
			.margin = st::boxRowPadding - style::margins(
				st::settingsDeviceName.textMargins.left(),
				0,
				st::settingsDeviceName.textMargins.right(),
				0),
		};
	}, [] {
		return SearchEntry{
			.id = u"ayu/localProfileAbout"_q,
			.title = tr::ayu_LocalProfileAbout(tr::now),
			.icon = { &st::menuIconInfo },
		};
	});
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_LocalProfilePhotoHeader());
	builder.addButton({
		.id = u"ayu/localProfilePhoto"_q,
		.title = tr::ayu_LocalProfilePhotoChoose(),
		.icon = { &st::menuIconProfile },
		.onClick = [=] {
			ChooseLocalProfilePhoto(controller);
		},
	});
	builder.addButton({
		.id = u"ayu/localProfilePhotoClear"_q,
		.title = tr::ayu_LocalProfilePhotoClear(),
		.icon = { &st::menuIconDelete },
		.onClick = [=] {
			AyuSettings::getInstance().setLocalProfilePhoto(QString());
			Ayu::RefreshLocalProfilePresentation(&controller->session());
			controller->showToast(tr::lng_box_done(tr::now));
		},
	});
	builder.addDividerText(tr::ayu_LocalProfileDescription());
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_LocalProfileCollectiblesHeader());
	ayu.addSettingToggle({
		.id = u"ayu/localProfileGiftEnabled"_q,
		.title = tr::ayu_LocalProfileGiftEnabled(),
		.getter = &AyuSettings::localProfileGiftEnabled,
		.setter = &AyuSettings::setLocalProfileGiftEnabled,
		.icon = { &st::menuIconGiftPremium },
	});
	builder.addButton({
		.id = u"ayu/localProfileGift"_q,
		.title = tr::ayu_LocalProfileGiftPickerChooseFeatured(),
		.icon = { &st::menuIconGiftPremium },
		.onClick = [=] {
			ShowLocalGiftCollectionPicker(
				controller,
				LocalGiftTarget::Featured);
		},
	});
	builder.addButton({
		.id = u"ayu/localProfileGifts"_q,
		.title = tr::ayu_LocalProfileGiftPickerManagePinned(),
		.icon = { &st::menuIconGiftPremium },
		.onClick = [=] {
			ShowLocalPinnedGiftManager(controller);
		},
	});
	builder.addButton({
		.id = u"ayu/localProfileGiftClear"_q,
		.title = tr::ayu_LocalProfileGiftPickerClearFeatured(),
		.icon = { &st::menuIconDelete },
		.onClick = [=] {
			AyuSettings::getInstance().setLocalProfileGift(QString());
			controller->showToast(tr::lng_box_done(tr::now));
		},
	});
	builder.addDividerText(tr::ayu_LocalProfileGiftDescription());
	builder.addDividerText(tr::ayu_LocalProfileCredit());
}

void BuildOtherThings(SectionBuilder &builder) {
	const auto controller = builder.controller();

#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	builder.addDivider();
#endif
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_MaintenanceHeader());
	builder.addButton({
		.id = u"ayu/registerUrlScheme"_q,
		.title = tr::ayu_RegisterURLScheme(),
		.icon = { &st::menuIconLink },
		.onClick = [=] {
			Core::Application::RegisterUrlScheme();
			controller->showToast(tr::lng_box_done(tr::now));
		},
	});
	builder.addButton({
		.id = u"ayu/resetSettings"_q,
		.title = tr::ayu_ResetSettings(),
		.icon = { &st::menuIconRestore },
		.onClick = [=] {
			controller->show(Ui::MakeConfirmBox({
				.text = tr::ayu_ResetSettingsConfirmation(tr::rich),
				.confirmed = [=](Fn<void()> &&close) {
					AyuSettings::reset();
					controller->showToast(tr::lng_box_done(tr::now));
					close();
				},
				.confirmText = tr::lng_box_yes(),
			}));
		},
	});
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuOther::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_CategoryLocalProfile,
	.icon = &st::menuIconProfile,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	BuildLocalProfile(builder, ayu);
});

const auto kAdvancedMeta = BuildHelper({
	.id = AyuAdvanced::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_CategoryAdvanced,
	.icon = &st::menuIconSettings,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	builder.addSkip();
	BuildCrashReporting(builder, ayu);
	BuildOtherThings(builder);
});

} // namespace

rpl::producer<QString> AyuOther::title() {
	return tr::ayu_CategoryLocalProfile();
}

AyuOther::AyuOther(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuOther::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

AyuAdvanced::AyuAdvanced(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> AyuAdvanced::title() {
	return tr::ayu_CategoryAdvanced();
}

void AyuAdvanced::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kAdvancedMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuOtherId() {
	return AyuOther::Id();
}

} // namespace Settings
