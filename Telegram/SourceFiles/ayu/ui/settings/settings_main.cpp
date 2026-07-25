// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_main.h"

#include "ayu/ui/settings/settings_appearance.h"
#include "ayu/ui/settings/settings_ayu.h"
#include "ayu/ui/settings/settings_chats.h"
#include "ayu/ui/settings/settings_filters.h"
#include "ayu/ui/settings/settings_general.h"
#include "ayu/ui/settings/settings_other.h"
#include "ayu/ui/settings/settings_vault.h"
#include "ayu/ui/settings/settings_visual_effects.h"
#include "ayu/ui/ayu_logo.h"
#include "ayu/ayu_settings.h"
#include "boxes/abstract_box.h"
#include "core/version.h"
#include "lang/lang_text_entity.h"
#include "settings/sections/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/painter.h"
#include "ui/vertical_list.h"
#include "window/window_session_controller.h"
#include "window/window_session_controller_link_info.h"
#include "lang_auto.h"

#include "styles/style_ayu_settings.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QDesktopServices>
#include <QUrl>

namespace Settings {

using namespace Builder;

namespace {

void BuildLogo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto logo = object_ptr<Ui::RpWidget>(ctx.container);
		const auto logoRaw = logo.data();
		logoRaw->resize(
			QSize(st::settingsCloudPasswordIconSize,
				st::settingsCloudPasswordIconSize));
		logoRaw->setNaturalWidth(st::settingsCloudPasswordIconSize);
		logoRaw->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(logoRaw);
			const auto image = AyuAssets::currentAppLogoPad();
			if (!image.isNull()) {
				const auto size = st::settingsCloudPasswordIconSize;
				const auto scaled = image.scaled(
					size * style::DevicePixelRatio(),
					size * style::DevicePixelRatio(),
					Qt::KeepAspectRatio,
					Qt::SmoothTransformation);
				p.drawImage(QRect(0, 0, size, size), scaled);
			}
		}, logoRaw->lifetime());
		return { .widget = std::move(logo), .align = style::al_top };
	});
}

void BuildVersionInfo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				rpl::single(
					QString("IntelGram Desktop v")
					+ QString::fromLatin1(AppVersionStr)),
				st::boxTitle),
			.align = style::al_top,
		};
	});

	builder.addSkip();

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				tr::ayu_SettingsDescription(),
				st::centeredBoxLabel),
			.align = style::al_top,
		};
	});
}

void BuildCategories(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::ayu_SettingsCustomizeHeader());

	builder.addSectionButton({
		.title = tr::ayu_CategoryLocalProfile(),
		.targetSection = AyuOther::Id(),
		.icon = { &st::menuIconProfile },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryAppearance(),
		.targetSection = AyuAppearance::Id(),
		.icon = { &st::menuIconPalette },
	});
	builder.addSectionButton({
		.title = tr::ayu_VisualEffectsTitle(),
		.targetSection = AyuVisualEffects::Id(),
		.icon = { &st::menuIconTransparent },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryNavigation(),
		.targetSection = AyuNavigation::Id(),
		.icon = { &st::menuIconShowInFolder },
	});

	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_SettingsPowerToolsHeader());

	builder.addSectionButton({
		.title = tr::ayu_VaultTitle(),
		.targetSection = AyuVault::Id(),
		.icon = { &st::menuIconArchive },
	});
	builder.addSectionButton({
		.title = tr::ayu_AutomationTitle(),
		.targetSection = AyuAutomation::Id(),
		.icon = { &st::menuIconBot },
	});
	builder.addSectionButton({
		.title = tr::ayu_ExportTitle(),
		.targetSection = AyuExport::Id(),
		.icon = { &st::menuIconExport },
	});

	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_SettingsClientHeader());

	builder.addSectionButton({
		.title = tr::ayu_CategoryPrivacy(),
		.targetSection = AyuGhost::Id(),
		.icon = { &st::menuIconLock },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryChats(),
		.targetSection = AyuChats::Id(),
		.icon = { &st::menuIconChatBubble },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryFilters(),
		.targetSection = AyuFilters::Id(),
		.icon = { &st::menuIconTagFilter },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryGeneral(),
		.targetSection = AyuGeneral::Id(),
		.icon = { &st::menuIconShowAll },
	});
	builder.addSectionButton({
		.title = tr::ayu_CategoryAdvanced(),
		.targetSection = AyuAdvanced::Id(),
		.icon = { &st::menuIconSettings },
	});
}

void ShowIntelGramUpdateLog(
		not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_IntelGramUpdateLogTitle());
		const auto content = box->verticalLayout();

		Ui::AddSubsectionTitle(
			content,
			tr::ayu_IntelGramUpdateLogCurrentTitle());
		Ui::AddDividerText(
			content,
			tr::ayu_IntelGramUpdateLogCurrent(tr::rich));
		Ui::AddSubsectionTitle(
			content,
			tr::ayu_IntelGramUpdateLogFeaturesTitle());
		Ui::AddDividerText(
			content,
			tr::ayu_IntelGramUpdateLogFeatures(tr::rich));
		Ui::AddSubsectionTitle(
			content,
			tr::ayu_IntelGramUpdateLogPrivacyTitle());
		Ui::AddDividerText(
			content,
			tr::ayu_IntelGramUpdateLogPrivacy(tr::rich));
		Ui::AddDividerText(
			content,
			tr::ayu_IntelGramUpdateLogCredit());

		box->addButton(tr::ayu_IntelGramUpdateLogFull(), [=] {
			QDesktopServices::openUrl(QUrl(
				u"https://github.com/foolspec/IntelGram/blob/main/CHANGELOG.md"_q));
		});
		box->addButton(tr::lng_box_done(), [=] {
			box->closeBox();
		});
	}));
}

void BuildLinks(SectionBuilder &builder) {
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::ayu_IntelGramLinksHeader());

	const auto controller = builder.controller();

	builder.addButton({
		.id = u"intelgram/telegram"_q,
		.title = tr::ayu_IntelGramLinksTelegram(),
		.icon = { &st::menuIconChannel },
		.label = rpl::single(u"@intelgrams"_q),
		.onClick = [=] {
			controller->showPeerByLink(Window::PeerByLinkInfo{
				.usernameOrId = u"intelgrams"_q,
			});
		},
	});
	builder.addButton({
		.id = u"intelgram/updateLog"_q,
		.title = tr::ayu_IntelGramLinksUpdateLog(),
		.icon = { &st::menuIconInfo },
		.label = tr::ayu_IntelGramUpdateLogLabel(),
		.onClick = [=] {
			ShowIntelGramUpdateLog(controller);
		},
	});
	builder.addButton({
		.id = u"intelgram/github"_q,
		.title = tr::ayu_IntelGramLinksGitHub(),
		.icon = { &st::menuIconIpAddress },
		.label = rpl::single(u"foolspec/IntelGram"_q),
		.onClick = [=] {
			QDesktopServices::openUrl(QUrl(
				u"https://github.com/foolspec/IntelGram"_q));
		},
	});
	builder.addButton({
		.id = u"intelgram/changelog"_q,
		.title = tr::ayu_IntelGramLinksChangelog(),
		.icon = { &st::menuIconShowAll },
		.label = rpl::single(u"CHANGELOG.md"_q),
		.onClick = [=] {
			QDesktopServices::openUrl(QUrl(
				u"https://github.com/foolspec/IntelGram/blob/main/CHANGELOG.md"_q));
		},
	});
	builder.addButton({
		.id = u"intelgram/technicalChangelog"_q,
		.title = tr::ayu_IntelGramLinksTechnicalChangelog(),
		.icon = { &st::menuIconChats },
		.label = rpl::single(u"TECHNICAL_CHANGELOG.md"_q),
		.onClick = [=] {
			QDesktopServices::openUrl(QUrl(
				u"https://github.com/foolspec/IntelGram/blob/main/TECHNICAL_CHANGELOG.md"_q));
		},
	});
	builder.addDividerText(tr::ayu_IntelGramLinksDescription());

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuMain::Id(),
	.parentId = MainId(),
	.title = &tr::ayu_AyuPreferences,
	.icon = &st::menuIconPremium,
}, [](SectionBuilder &builder) {
	BuildLogo(builder);
	builder.addSkip();
	BuildVersionInfo(builder);
	BuildCategories(builder);
	BuildLinks(builder);
});

} // namespace

rpl::producer<QString> AyuMain::title() {
	return rpl::single(QString(""));
}

AyuMain::AyuMain(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuMain::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuMainId() {
	return AyuMain::Id();
}

} // namespace Settings
