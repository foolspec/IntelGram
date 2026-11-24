#include "ayu/ui/local_channel_workspace.h"

#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/utils/ayu_profile_values.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "data/data_channel.h"
#include "data/data_session.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "ui/boxes/confirm_box.h"
#include "ui/controls/userpic_button.h"
#include "ui/image/image.h"
#include "ui/layers/generic_box.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include "styles/style_boxes.h"
#include "styles/style_info.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <QtCore/QPointer>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

namespace Ayu {
namespace {

constexpr auto kMaxLocalChannels = 8;
constexpr auto kMaxLocalChannelPosts = 100;
constexpr auto kMaxLocalChannelPostLength = 4096;
constexpr auto kMaxLocalChannelTitleLength = 128;
constexpr auto kMaxLocalChannelAboutLength = 512;
constexpr auto kMaxLocalChannelPathLength = 2048;
constexpr auto kMaxLocalChannelWorkspaceLength = 1'400'000;

struct LocalChannelPost {
	qint64 id = 0;
	qint64 createdAt = 0;
	qint64 editedAt = 0;
	QString text;
	QString attachment;
	bool pinned = false;
};

struct LocalChannelState {
	QString title;
	QString about;
	QString photo;
	qint64 updatedAt = 0;
	std::vector<LocalChannelPost> posts;
};

struct LocalChannelComposerState {
	QString attachment;
	rpl::variable<QString> attachmentLabel;
};

struct LocalChannelSettingsState {
	QString photo;
	rpl::variable<QString> photoLabel;
};

[[nodiscard]] QString CurrentChannelUsername() {
	return NormalizeLocalProfileUsername(
		AyuSettings::localProfile().localProfileChannel());
}

[[nodiscard]] QJsonObject ReadWorkspaceRoot() {
	const auto encoded = AyuSettings::localProfile(
		).localProfileChannelWorkspace().toUtf8();
	if (encoded.isEmpty()) {
		return {};
	}
	QJsonParseError error;
	const auto document = QJsonDocument::fromJson(encoded, &error);
	return (error.error == QJsonParseError::NoError && document.isObject())
		? document.object()
		: QJsonObject();
}

[[nodiscard]] LocalChannelPost ReadPost(const QJsonObject &object) {
	return {
		.id = object.value(u"id"_q).toString().toLongLong(),
		.createdAt = object.value(u"createdAt"_q).toString().toLongLong(),
		.editedAt = object.value(u"editedAt"_q).toString().toLongLong(),
		.text = object.value(u"text"_q).toString().left(
			kMaxLocalChannelPostLength),
		.attachment = object.value(u"attachment"_q).toString().left(
			kMaxLocalChannelPathLength),
		.pinned = object.value(u"pinned"_q).toBool(),
	};
}

[[nodiscard]] QJsonObject WritePost(const LocalChannelPost &post) {
	return {
		{ u"id"_q, QString::number(post.id) },
		{ u"createdAt"_q, QString::number(post.createdAt) },
		{ u"editedAt"_q, QString::number(post.editedAt) },
		{ u"text"_q, post.text.left(kMaxLocalChannelPostLength) },
		{ u"attachment"_q, post.attachment.left(
			kMaxLocalChannelPathLength) },
		{ u"pinned"_q, post.pinned },
	};
}

[[nodiscard]] LocalChannelState ReadChannelState() {
	const auto username = CurrentChannelUsername();
	if (username.isEmpty()) {
		return {};
	}
	const auto root = ReadWorkspaceRoot();
	const auto object = root.value(u"channels"_q).toObject(
		).value(username).toObject();
	auto result = LocalChannelState{
		.title = object.value(u"title"_q).toString().left(
			kMaxLocalChannelTitleLength),
		.about = object.value(u"about"_q).toString().left(
			kMaxLocalChannelAboutLength),
		.photo = object.value(u"photo"_q).toString().left(
			kMaxLocalChannelPathLength),
		.updatedAt = object.value(u"updatedAt"_q).toString().toLongLong(),
	};
	const auto posts = object.value(u"posts"_q).toArray();
	result.posts.reserve(std::min(
		int(posts.size()),
		kMaxLocalChannelPosts));
	for (const auto &value : posts) {
		if (result.posts.size() >= kMaxLocalChannelPosts) {
			break;
		}
		const auto post = ReadPost(value.toObject());
		if (post.id && (!post.text.isEmpty() || !post.attachment.isEmpty())) {
			result.posts.push_back(post);
		}
	}
	return result;
}

[[nodiscard]] QJsonObject WriteChannelState(
		const LocalChannelState &state) {
	auto posts = QJsonArray();
	const auto from = (state.posts.size() > kMaxLocalChannelPosts)
		? (state.posts.size() - kMaxLocalChannelPosts)
		: 0;
	for (auto i = from; i != state.posts.size(); ++i) {
		posts.push_back(WritePost(state.posts[i]));
	}
	return {
		{ u"title"_q, state.title.left(kMaxLocalChannelTitleLength) },
		{ u"about"_q, state.about.left(kMaxLocalChannelAboutLength) },
		{ u"photo"_q, state.photo.left(kMaxLocalChannelPathLength) },
		{ u"updatedAt"_q, QString::number(state.updatedAt) },
		{ u"posts"_q, posts },
	};
}

void SaveChannelState(LocalChannelState state) {
	const auto username = CurrentChannelUsername();
	if (username.isEmpty()) {
		return;
	}
	state.updatedAt = QDateTime::currentSecsSinceEpoch();
	auto root = ReadWorkspaceRoot();
	auto channels = root.value(u"channels"_q).toObject();
	channels.insert(username, WriteChannelState(state));
	while (channels.size() > kMaxLocalChannels) {
		auto oldest = QString();
		auto oldestTime = std::numeric_limits<qint64>::max();
		for (auto i = channels.begin(); i != channels.end(); ++i) {
			if (i.key() == username) {
				continue;
			}
			const auto updated = i.value().toObject(
				).value(u"updatedAt"_q).toString().toLongLong();
			if (updated < oldestTime) {
				oldest = i.key();
				oldestTime = updated;
			}
		}
		if (oldest.isEmpty()) {
			break;
		}
		channels.remove(oldest);
	}
	root.insert(u"version"_q, 1);
	root.insert(u"channels"_q, channels);
	auto encoded = QString::fromUtf8(
		QJsonDocument(root).toJson(QJsonDocument::Compact));
	while (encoded.size() > kMaxLocalChannelWorkspaceLength
		&& channels.size() > 1) {
		for (auto i = channels.begin(); i != channels.end(); ++i) {
			if (i.key() != username) {
				channels.erase(i);
				break;
			}
		}
		root.insert(u"channels"_q, channels);
		encoded = QString::fromUtf8(
			QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
	while (encoded.size() > kMaxLocalChannelWorkspaceLength
		&& !state.posts.empty()) {
		state.posts.erase(state.posts.begin());
		channels.insert(username, WriteChannelState(state));
		root.insert(u"channels"_q, channels);
		encoded = QString::fromUtf8(
			QJsonDocument(root).toJson(QJsonDocument::Compact));
	}
	AyuSettings::localProfile().setLocalProfileChannelWorkspace(encoded);
}

[[nodiscard]] QString ChannelTitle(
		ChannelData *channel,
		const LocalChannelState &state) {
	if (!state.title.trimmed().isEmpty()) {
		return state.title.trimmed();
	}
	return channel ? channel->name() : CurrentChannelUsername();
}

[[nodiscard]] QString AttachmentLabel(const QString &path) {
	return path.isEmpty()
		? tr::ayu_LocalChannelWorkspaceAttachmentNone(tr::now)
		: QFileInfo(path).fileName();
}

[[nodiscard]] QString PostTitle(const LocalChannelPost &post) {
	auto result = post.text.simplified();
	if (result.isEmpty()) {
		result = AttachmentLabel(post.attachment);
	}
	if (result.size() > 96) {
		result = result.left(93) + u"..."_q;
	}
	return post.pinned
		? (tr::ayu_LocalChannelWorkspacePinned(tr::now)
			+ u": "_q
			+ result)
		: result;
}

[[nodiscard]] QString PostDetails(const LocalChannelPost &post) {
	auto parts = QStringList();
	const auto date = QDateTime::fromSecsSinceEpoch(
		post.createdAt
	).toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
	if (!date.isEmpty()) {
		parts.push_back(date);
	}
	if (post.editedAt) {
		parts.push_back(
			tr::ayu_LocalChannelWorkspaceEdited(tr::now));
	}
	if (!post.attachment.isEmpty()) {
		parts.push_back(AttachmentLabel(post.attachment));
	}
	return parts.join(u" - "_q);
}

[[nodiscard]] LocalChannelPost *FindPost(
		LocalChannelState &state,
		qint64 id) {
	for (auto &post : state.posts) {
		if (post.id == id) {
			return &post;
		}
	}
	return nullptr;
}

void ShowChannelBox(
	not_null<Window::SessionController*> controller,
	not_null<ChannelData*> channel);
void ShowChannelSettings(
	not_null<Window::SessionController*> controller,
	not_null<ChannelData*> channel);
void ShowPostActions(
	not_null<Window::SessionController*> controller,
	not_null<ChannelData*> channel,
	qint64 postId);
void ShowPostComposer(
	not_null<Window::SessionController*> controller,
	not_null<ChannelData*> channel,
	qint64 postId);

void ResolveChannel(
		not_null<Window::SessionController*> controller,
		Fn<void(not_null<ChannelData*>)> done) {
	const auto username = CurrentChannelUsername();
	if (username.isEmpty()) {
		controller->showToast(
			tr::ayu_LocalChannelWorkspaceMissing(tr::now));
		return;
	}
	if (const auto peer = controller->session().data().peerByUsername(
			username)) {
		if (const auto channel = peer->asChannel()) {
			done(channel);
			return;
		}
	}
	const auto weak = base::make_weak(controller);
	controller->session().api().request(
		MTPcontacts_ResolveUsername(
			MTP_flags(0),
			MTP_string(username),
			MTP_string())
	).done([=](const MTPcontacts_ResolvedPeer &result) {
		if (!weak) {
			return;
		}
		const auto &data = result.data();
		auto &owner = weak->session().data();
		owner.processUsers(data.vusers());
		owner.processChats(data.vchats());
		const auto peer = owner.peerLoaded(peerFromMTP(data.vpeer()));
		if (const auto channel = peer ? peer->asChannel() : nullptr) {
			done(channel);
		} else {
			weak->showToast(
				tr::ayu_LocalProfileChannelNotFound(tr::now));
		}
	}).fail([=](const MTP::Error &) {
		if (weak) {
			weak->showToast(
				tr::ayu_LocalProfileChannelNotFound(tr::now));
		}
	}).send();
}

void ShowPostComposer(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel,
		qint64 postId) {
	auto saved = ReadChannelState();
	const auto existing = postId ? FindPost(saved, postId) : nullptr;
	const auto initialText = existing ? existing->text : QString();
	const auto initialAttachment = existing
		? existing->attachment
		: QString();
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(postId
			? tr::ayu_LocalChannelWorkspaceEditPost()
			: tr::ayu_LocalChannelWorkspaceNewPost());
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				Ui::InputField::Mode::MultiLine,
				tr::ayu_LocalChannelWorkspacePostPlaceholder(),
				initialText),
			st::boxRowPadding);
		field->setMaxLength(kMaxLocalChannelPostLength);
		const auto composer = box->lifetime(
			).make_state<LocalChannelComposerState>();
		composer->attachment = initialAttachment;
		composer->attachmentLabel = AttachmentLabel(initialAttachment);
		Ui::AddDividerText(
			box->verticalLayout(),
			composer->attachmentLabel.value());
		box->setFocusCallback([=] {
			field->setFocusFast();
		});
		const auto weak = QPointer<Ui::GenericBox>(box);
		box->addButton(
			tr::ayu_LocalChannelWorkspaceAttachment(),
			[=] {
				FileDialog::GetOpenPath(
					Core::App().getFileDialogParent(),
					tr::ayu_LocalChannelWorkspaceAttachment(tr::now),
					FileDialog::AllFilesFilter(),
					[=](const FileDialog::OpenResult &result) {
						if (!weak || result.paths.isEmpty()) {
							return;
						}
						composer->attachment = result.paths.front().left(
							kMaxLocalChannelPathLength);
						composer->attachmentLabel = AttachmentLabel(
							composer->attachment);
					});
			});
		const auto save = [=] {
			auto state = ReadChannelState();
			const auto text = field->getLastText().trimmed().left(
				kMaxLocalChannelPostLength);
			const auto attachment = composer->attachment.left(
				kMaxLocalChannelPathLength);
			if (text.isEmpty() && attachment.isEmpty()) {
				field->showError();
				return;
			}
			const auto now = QDateTime::currentSecsSinceEpoch();
			if (postId) {
				if (const auto post = FindPost(state, postId)) {
					post->text = text;
					post->attachment = attachment;
					post->editedAt = now;
				}
			} else {
				state.posts.push_back({
					.id = QDateTime::currentMSecsSinceEpoch(),
					.createdAt = now,
					.editedAt = 0,
					.text = text,
					.attachment = attachment,
					.pinned = false,
				});
				while (state.posts.size() > kMaxLocalChannelPosts) {
					state.posts.erase(state.posts.begin());
				}
			}
			SaveChannelState(std::move(state));
			box->closeBox();
			controller->showToast(
				tr::ayu_LocalChannelWorkspacePostSaved(tr::now));
			ShowChannelBox(controller, channel);
		};
		box->addButton(
			tr::ayu_LocalChannelWorkspacePostSave(),
			save);
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
			ShowChannelBox(controller, channel);
		});
	}));
}

void ShowPostActions(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel,
		qint64 postId) {
	auto state = ReadChannelState();
	const auto found = FindPost(state, postId);
	if (!found) {
		ShowChannelBox(controller, channel);
		return;
	}
	const auto post = *found;
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::ayu_LocalChannelWorkspacePostTitle());
		Ui::AddDividerText(
			box->verticalLayout(),
			rpl::single(post.text.isEmpty()
				? AttachmentLabel(post.attachment)
				: post.text));
		Ui::AddDividerText(
			box->verticalLayout(),
			rpl::single(PostDetails(post)));
		box->addButton(
			tr::ayu_LocalChannelWorkspaceEditPost(),
			[=] {
				box->closeBox();
				ShowPostComposer(controller, channel, postId);
			});
		box->addButton(
			post.pinned
				? tr::ayu_LocalChannelWorkspaceUnpin()
				: tr::ayu_LocalChannelWorkspacePin(),
			[=] {
				auto changed = ReadChannelState();
				if (const auto current = FindPost(changed, postId)) {
					current->pinned = !current->pinned;
					SaveChannelState(std::move(changed));
				}
				box->closeBox();
				ShowChannelBox(controller, channel);
			});
		if (!post.text.isEmpty()) {
			box->addButton(
				tr::ayu_LocalChannelWorkspaceCopy(),
				[=] {
					QGuiApplication::clipboard()->setText(post.text);
					controller->showToast(
						tr::lng_text_copied(tr::now));
				});
		}
		if (!post.attachment.isEmpty()
			&& QFileInfo::exists(post.attachment)) {
			box->addButton(
				tr::ayu_LocalChannelWorkspaceAttachmentOpen(),
				[=] {
					File::Launch(post.attachment);
				});
		}
		box->addButton(
			tr::ayu_LocalChannelWorkspaceDeletePost(),
			[=] {
				box->closeBox();
				controller->show(Ui::MakeConfirmBox({
					.text = tr::ayu_LocalChannelWorkspaceDeleteConfirm(),
					.confirmed = [=](Fn<void()> close) {
						auto changed = ReadChannelState();
						changed.posts.erase(
							std::remove_if(
								changed.posts.begin(),
								changed.posts.end(),
								[=](const LocalChannelPost &entry) {
									return entry.id == postId;
								}),
							changed.posts.end());
						SaveChannelState(std::move(changed));
						close();
						ShowChannelBox(controller, channel);
					},
					.confirmText = tr::lng_box_delete(),
					.cancelText = tr::lng_cancel(),
					.confirmStyle = &st::attentionBoxButton,
				}));
			});
		box->addButton(
			tr::ayu_LocalChannelWorkspaceBack(),
			[=] {
				box->closeBox();
				ShowChannelBox(controller, channel);
			});
	}));
}

void ShowChannelSettings(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel) {
	const auto saved = ReadChannelState();
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::ayu_LocalChannelWorkspaceSettingsTitle());
		const auto title = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::ayu_LocalChannelWorkspaceTitleField(),
				saved.title),
			st::boxRowPadding);
		title->setMaxLength(kMaxLocalChannelTitleLength);
		const auto about = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				Ui::InputField::Mode::MultiLine,
				tr::ayu_LocalChannelWorkspaceAboutField(),
				saved.about),
			st::boxRowPadding);
		about->setMaxLength(kMaxLocalChannelAboutLength);
		const auto settings = box->lifetime(
			).make_state<LocalChannelSettingsState>();
		settings->photo = saved.photo;
		settings->photoLabel = AttachmentLabel(saved.photo);
		Ui::AddDividerText(
			box->verticalLayout(),
			settings->photoLabel.value());
		Ui::AddDividerText(
			box->verticalLayout(),
			tr::ayu_LocalChannelWorkspaceLocalOnly());
		const auto weak = QPointer<Ui::GenericBox>(box);
		box->addButton(
			tr::ayu_LocalChannelWorkspacePhotoChoose(),
			[=] {
				const auto filter =
					u"Image files (*.png *.jpg *.jpeg *.webp *.heic);;"_q
					+ FileDialog::AllFilesFilter();
				FileDialog::GetOpenPath(
					Core::App().getFileDialogParent(),
					tr::ayu_LocalChannelWorkspacePhotoChoose(tr::now),
					filter,
					[=](const FileDialog::OpenResult &result) {
						if (!weak || result.paths.isEmpty()) {
							return;
						}
						const auto path = result.paths.front();
						if (Images::Read({ .path = path }).image.isNull()) {
							controller->showToast(
								tr::ayu_LocalChannelWorkspacePhotoInvalid(
									tr::now));
							return;
						}
						settings->photo = path.left(
							kMaxLocalChannelPathLength);
						settings->photoLabel = AttachmentLabel(
							settings->photo);
					});
			});
		box->addButton(
			tr::ayu_LocalChannelWorkspacePhotoClear(),
			[=] {
				settings->photo = QString();
				settings->photoLabel = AttachmentLabel(QString());
			});
		box->addButton(
			tr::ayu_LocalChannelWorkspaceSettingsSave(),
			[=] {
				auto state = ReadChannelState();
				state.title = title->getLastText().trimmed().left(
					kMaxLocalChannelTitleLength);
				state.about = about->getLastText().trimmed().left(
					kMaxLocalChannelAboutLength);
				state.photo = settings->photo.left(
					kMaxLocalChannelPathLength);
				SaveChannelState(std::move(state));
				box->closeBox();
				controller->showToast(
					tr::ayu_LocalChannelWorkspaceSettingsSaved(tr::now));
				ShowChannelBox(controller, channel);
			});
		box->addButton(
			tr::ayu_LocalChannelWorkspaceSettingsReset(),
			[=] {
				auto state = ReadChannelState();
				state.title = QString();
				state.about = QString();
				state.photo = QString();
				SaveChannelState(std::move(state));
				box->closeBox();
				ShowChannelBox(controller, channel);
			});
		if (!saved.posts.empty()) {
			box->addButton(
				tr::ayu_LocalChannelWorkspacePostsClear(),
				[=] {
					box->closeBox();
					controller->show(Ui::MakeConfirmBox({
						.text = tr::ayu_LocalChannelWorkspacePostsClearConfirm(),
						.confirmed = [=](Fn<void()> close) {
							auto state = ReadChannelState();
							state.posts.clear();
							SaveChannelState(std::move(state));
							close();
							ShowChannelBox(controller, channel);
						},
						.confirmText = tr::lng_box_delete(),
						.cancelText = tr::lng_cancel(),
						.confirmStyle = &st::attentionBoxButton,
					}));
				});
		}
		box->addButton(
			tr::ayu_LocalChannelWorkspaceBack(),
			[=] {
				box->closeBox();
				ShowChannelBox(controller, channel);
			});
	}));
}

void ShowChannelBox(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel) {
	const auto state = ReadChannelState();
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(rpl::single(ChannelTitle(channel, state)));
		const auto content = box->verticalLayout();
		Ui::AddSubsectionTitle(
			content,
			tr::ayu_LocalChannelWorkspaceTitle());
		Ui::AddDividerText(
			content,
			tr::ayu_LocalChannelWorkspaceLocalOnly());
		const auto userpic = content->add(
			object_ptr<Ui::UserpicButton>(
				content,
				channel,
				st::infoPersonalChannelUserpic),
			style::al_top);
		if (!state.photo.isEmpty()) {
			auto image = Images::Read({ .path = state.photo }).image;
			if (!image.isNull()) {
				userpic->showCustom(std::move(image));
			}
		}
		userpic->setAttribute(Qt::WA_TransparentForMouseEvents);
		const auto settings = content->add(
			object_ptr<Ui::SettingsButton>(
				content,
				tr::ayu_LocalChannelWorkspaceSettings(),
				st::settingsButtonNoIcon));
		settings->setClickedCallback([=] {
			box->closeBox();
			ShowChannelSettings(controller, channel);
		});
		if (!state.about.isEmpty()) {
			Ui::AddDividerText(content, rpl::single(state.about));
		}
		const auto pinned = std::find_if(
			state.posts.rbegin(),
			state.posts.rend(),
			[](const LocalChannelPost &post) {
				return post.pinned;
			});
		if (pinned != state.posts.rend()) {
			Ui::AddSubsectionTitle(
				content,
				tr::ayu_LocalChannelWorkspacePinned());
			const auto button = content->add(
				object_ptr<Ui::SettingsButton>(
					content,
					rpl::single(PostTitle(*pinned)),
					st::settingsButtonNoIcon));
			const auto postId = pinned->id;
			button->setClickedCallback([=] {
				box->closeBox();
				ShowPostActions(controller, channel, postId);
			});
			Ui::AddDividerText(
				content,
				rpl::single(PostDetails(*pinned)));
		}
		Ui::AddSubsectionTitle(
			content,
			tr::ayu_LocalChannelWorkspacePosts());
		if (state.posts.empty()) {
			Ui::AddDividerText(
				content,
				tr::ayu_LocalChannelWorkspaceEmpty());
		}
		for (auto i = state.posts.rbegin(); i != state.posts.rend(); ++i) {
			const auto post = *i;
			const auto button = content->add(
				object_ptr<Ui::SettingsButton>(
					content,
					rpl::single(PostTitle(post)),
					st::settingsButtonNoIcon));
			button->setClickedCallback([=] {
				box->closeBox();
				ShowPostActions(controller, channel, post.id);
			});
			Ui::AddDividerText(
				content,
				rpl::single(PostDetails(post)));
		}
		box->addButton(
			tr::ayu_LocalChannelWorkspaceNewPost(),
			[=] {
				box->closeBox();
				ShowPostComposer(controller, channel, 0);
			});
		box->addButton(
			tr::ayu_LocalChannelWorkspaceSettings(),
			[=] {
				box->closeBox();
				ShowChannelSettings(controller, channel);
			});
		box->addButton(tr::lng_box_done(), [=] {
			box->closeBox();
		});
	}));
}

} // namespace

QString LocalChannelDisplayTitle(ChannelData *channel) {
	return ChannelTitle(channel, ReadChannelState());
}

void ShowLocalChannelWorkspace(
		not_null<Window::SessionController*> controller) {
	ResolveChannel(controller, [=](not_null<ChannelData*> channel) {
		ShowChannelBox(controller, channel);
	});
}

} // namespace Ayu
