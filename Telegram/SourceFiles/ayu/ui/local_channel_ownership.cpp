#include "ayu/ui/local_channel_ownership.h"

#include "ayu/data/intelgram_vault.h"
#include "base/unixtime.h"
#include "data/data_changes.h"
#include "data/data_channel.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/text/text_entity.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace Ayu {
namespace {

constexpr auto kMaxPosts = 100;
constexpr auto kMaxPostLength = 4096;
constexpr auto kMaxTagIdLength = 512;
constexpr auto kMaxStoredStateLength = 1'000'000;

struct LocalBroadcast {
	qint64 uid = 0;
	TimeId date = 0;
	TextWithTags text;
};

struct LocalOwnershipState {
	bool enabled = false;
	std::vector<LocalBroadcast> posts;
};

struct RuntimeBroadcast {
	qint64 uid = 0;
	FullMsgId fullId;
};

[[nodiscard]] QString StorageKey(not_null<ChannelData*> channel) {
	return u"local_channel_ownership/%1/%2"_q
		.arg(QString::number(channel->session().userId().bare))
		.arg(QString::number(channel->id.value));
}

[[nodiscard]] TextWithTags ReadText(const QJsonObject &object) {
	auto result = TextWithTags{
		.text = object.value(u"text"_q).toString().left(kMaxPostLength),
	};
	const auto tags = object.value(u"tags"_q).toArray();
	for (const auto &value : tags) {
		const auto tag = value.toObject();
		const auto offset = tag.value(u"offset"_q).toInt(-1);
		const auto length = tag.value(u"length"_q).toInt();
		const auto id = tag.value(u"id"_q).toString().left(kMaxTagIdLength);
		if (offset >= 0
			&& length > 0
			&& offset + length <= result.text.size()
			&& !id.isEmpty()) {
			result.tags.push_back({
				.offset = offset,
				.length = length,
				.id = id,
			});
		}
	}
	return result;
}

[[nodiscard]] QJsonObject WriteText(const TextWithTags &text) {
	auto tags = QJsonArray();
	for (const auto &tag : text.tags) {
		if (tag.offset < 0
			|| tag.length <= 0
			|| tag.offset + tag.length > text.text.size()
			|| tag.id.isEmpty()) {
			continue;
		}
		tags.push_back(QJsonObject{
			{ u"offset"_q, tag.offset },
			{ u"length"_q, tag.length },
			{ u"id"_q, tag.id.left(kMaxTagIdLength) },
		});
	}
	return {
		{ u"text"_q, text.text.left(kMaxPostLength) },
		{ u"tags"_q, tags },
	};
}

[[nodiscard]] LocalOwnershipState ParseState(const QString &encoded) {
	if (encoded.isEmpty() || encoded.size() > kMaxStoredStateLength) {
		return {};
	}
	QJsonParseError error;
	const auto document = QJsonDocument::fromJson(encoded.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		return {};
	}
	const auto object = document.object();
	auto result = LocalOwnershipState{
		.enabled = object.value(u"enabled"_q).toBool(),
	};
	const auto posts = object.value(u"posts"_q).toArray();
	result.posts.reserve(std::min(int(posts.size()), kMaxPosts));
	for (const auto &value : posts) {
		if (result.posts.size() >= kMaxPosts) {
			break;
		}
		const auto post = value.toObject();
		auto parsed = LocalBroadcast{
			.uid = post.value(u"uid"_q).toString().toLongLong(),
			.date = TimeId(post.value(u"date"_q).toInt()),
			.text = ReadText(post),
		};
		if (parsed.uid && parsed.date && !parsed.text.text.trimmed().isEmpty()) {
			result.posts.push_back(std::move(parsed));
		}
	}
	return result;
}

[[nodiscard]] QByteArray SerializeState(const LocalOwnershipState &state) {
	auto posts = QJsonArray();
	for (const auto &post : state.posts) {
		auto object = WriteText(post.text);
		object.insert(u"uid"_q, QString::number(post.uid));
		object.insert(u"date"_q, int(post.date));
		posts.push_back(object);
	}
	return QJsonDocument(QJsonObject{
		{ u"version"_q, 1 },
		{ u"enabled"_q, state.enabled },
		{ u"posts"_q, posts },
	}).toJson(QJsonDocument::Compact);
}

[[nodiscard]] std::map<QString, LocalOwnershipState> &StoredStates() {
	static auto result = std::map<QString, LocalOwnershipState>();
	return result;
}

[[nodiscard]] LocalOwnershipState &StateFor(
		not_null<ChannelData*> channel) {
	const auto key = StorageKey(channel);
	auto &states = StoredStates();
	const auto i = states.find(key);
	if (i != states.end()) {
		return i->second;
	}
	return states.emplace(
		key,
		ParseState(IntelGram::Vault::Option(key))
	).first->second;
}

void SaveState(
		not_null<ChannelData*> channel,
		LocalOwnershipState state) {
	while (state.posts.size() > kMaxPosts) {
		state.posts.erase(state.posts.begin());
	}
	auto encoded = SerializeState(state);
	while (encoded.size() > kMaxStoredStateLength && !state.posts.empty()) {
		state.posts.erase(state.posts.begin());
		encoded = SerializeState(state);
	}
	const auto key = StorageKey(channel);
	StoredStates()[key] = std::move(state);
	IntelGram::Vault::SetOption(key, QString::fromUtf8(encoded));
}

[[nodiscard]] std::map<History*, std::vector<RuntimeBroadcast>>
		&RuntimeBroadcasts() {
	static auto result
		= std::map<History*, std::vector<RuntimeBroadcast>>();
	return result;
}

void CleanRuntimeBroadcasts(not_null<History*> history) {
	const auto i = RuntimeBroadcasts().find(history);
	if (i == RuntimeBroadcasts().end()) {
		return;
	}
	auto &runtime = i->second;
	runtime.erase(
		std::remove_if(
			runtime.begin(),
			runtime.end(),
			[=](const RuntimeBroadcast &entry) {
				return !history->owner().message(entry.fullId);
			}),
		runtime.end());
}

void AddRuntimeBroadcast(
		not_null<History*> history,
		const LocalBroadcast &post) {
	auto text = TextWithEntities{
		post.text.text,
		TextUtilities::ConvertTextTagsToEntities(post.text.tags),
	};
	TextUtilities::Trim(text);
	const auto item = history->addNewLocalMessage({
		.id = history->owner().nextLocalMessageId(),
		.flags = MessageFlag::Post | MessageFlag::HasViews,
		.date = post.date,
	}, text, MTP_messageMediaEmpty());
	RuntimeBroadcasts()[history].push_back({
		.uid = post.uid,
		.fullId = item->fullId(),
	});
}

void RemoveRuntimeBroadcasts(not_null<History*> history) {
	const auto i = RuntimeBroadcasts().find(history);
	if (i == RuntimeBroadcasts().end()) {
		return;
	}
	const auto entries = i->second;
	RuntimeBroadcasts().erase(i);
	for (const auto &entry : entries) {
		if (const auto item = history->owner().message(entry.fullId)) {
			item->destroy();
		}
	}
}

void ShowToast(not_null<History*> history, const QString &text) {
	if (const auto window = history->session().tryResolveWindow()) {
		window->showToast(text);
	}
}

void NotifyOwnershipChanged(
		not_null<ChannelData*> channel,
		bool enabled) {
	const auto history = channel->owner().history(channel);
	if (enabled) {
		EnsureLocalChannelBroadcasts(history);
	} else {
		RemoveRuntimeBroadcasts(history);
	}
	channel->session().changes().peerUpdated(
		channel,
		Data::PeerUpdate::Flag::Rights
			| Data::PeerUpdate::Flag::FullInfo);
	channel->session().changes().historyUpdated(
		history,
		Data::HistoryUpdate::Flag::ClientSideMessages);
}

void SetOwnershipEnabled(
		not_null<ChannelData*> channel,
		bool enabled) {
	auto state = StateFor(channel);
	if (state.enabled == enabled) {
		return;
	}
	state.enabled = enabled;
	SaveState(channel, std::move(state));
	NotifyOwnershipChanged(channel, enabled);
}

void ClearBroadcasts(not_null<ChannelData*> channel) {
	auto state = StateFor(channel);
	state.posts.clear();
	SaveState(channel, std::move(state));
	RemoveRuntimeBroadcasts(channel->owner().history(channel));
}

[[nodiscard]] std::optional<qint64> RuntimeUid(HistoryItem *item) {
	if (!item) {
		return std::nullopt;
	}
	const auto history = item->history();
	CleanRuntimeBroadcasts(history);
	const auto i = RuntimeBroadcasts().find(history);
	if (i == RuntimeBroadcasts().end()) {
		return std::nullopt;
	}
	const auto fullId = item->fullId();
	const auto found = std::find_if(
		i->second.begin(),
		i->second.end(),
		[&](const RuntimeBroadcast &entry) {
			return entry.fullId == fullId;
		});
	return (found == i->second.end())
		? std::nullopt
		: std::make_optional(found->uid);
}

} // namespace

bool LocalChannelOwnershipActive(PeerData *peer) {
	const auto channel = peer ? peer->asBroadcast() : nullptr;
	return channel && StateFor(channel).enabled;
}

void EnsureLocalChannelBroadcasts(not_null<History*> history) {
	const auto channel = history->peer->asBroadcast();
	if (!channel || !StateFor(channel).enabled) {
		return;
	}
	CleanRuntimeBroadcasts(history);
	auto &runtime = RuntimeBroadcasts()[history];
	auto added = false;
	for (const auto &post : StateFor(channel).posts) {
		const auto exists = std::any_of(
			runtime.begin(),
			runtime.end(),
			[&](const RuntimeBroadcast &entry) {
				return entry.uid == post.uid;
			});
		if (!exists) {
			AddRuntimeBroadcast(history, post);
			added = true;
		}
	}
	if (added) {
		history->session().changes().historyUpdated(
			history,
			Data::HistoryUpdate::Flag::MessageSent);
	}
}

LocalChannelBroadcastResult HandleLocalChannelBroadcast(
		not_null<History*> history,
		const TextWithTags &text,
		bool scheduled) {
	const auto channel = history->peer->asBroadcast();
	if (!channel || !StateFor(channel).enabled) {
		return LocalChannelBroadcastResult::NotHandled;
	} else if (scheduled) {
		ShowToast(
			history,
			tr::ayu_LocalChannelOwnershipScheduleBlocked(tr::now));
		return LocalChannelBroadcastResult::Blocked;
	}
	auto prepared = TextWithEntities{
		text.text,
		TextUtilities::ConvertTextTagsToEntities(text.tags),
	};
	TextUtilities::Trim(prepared);
	if (prepared.text.isEmpty()) {
		ShowToast(
			history,
			tr::ayu_LocalChannelOwnershipTextOnly(tr::now));
		return LocalChannelBroadcastResult::Blocked;
	}
	const auto preserveTags = prepared.text.size() <= kMaxPostLength;
	auto storedText = TextWithTags{
		.text = prepared.text.left(kMaxPostLength),
		.tags = preserveTags
			? TextUtilities::ConvertEntitiesToTextTags(prepared.entities)
			: TextWithTags::Tags(),
	};
	auto state = StateFor(channel);
	auto uid = QDateTime::currentMSecsSinceEpoch();
	while (std::any_of(
			state.posts.begin(),
			state.posts.end(),
			[&](const LocalBroadcast &post) {
				return post.uid == uid;
			})) {
		++uid;
	}
	auto post = LocalBroadcast{
		.uid = uid,
		.date = base::unixtime::now(),
		.text = std::move(storedText),
	};
	state.posts.push_back(post);
	SaveState(channel, std::move(state));
	AddRuntimeBroadcast(history, post);
	history->session().changes().historyUpdated(
		history,
		Data::HistoryUpdate::Flag::MessageSent);
	return LocalChannelBroadcastResult::Saved;
}

bool InterceptLocalChannelUnsupportedSend(
		not_null<Window::SessionController*> controller,
		PeerData *peer) {
	if (!LocalChannelOwnershipActive(peer)) {
		return false;
	}
	controller->showToast(tr::ayu_LocalChannelOwnershipTextOnly(tr::now));
	return true;
}

void ShowLocalChannelOwnership(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);
		box->setTitle(tr::ayu_LocalChannelOwnershipTitle());
		const auto content = box->verticalLayout();
		const auto toggle = content->add(
			object_ptr<Ui::SettingsButton>(
				content,
				tr::ayu_LocalChannelOwnershipToggle(),
				st::settingsButtonNoIcon));
		toggle->toggleOn(rpl::single(StateFor(channel).enabled));
		toggle->toggledChanges(
		) | rpl::on_next([=](bool enabled) {
			SetOwnershipEnabled(channel, enabled);
			controller->showToast(enabled
				? tr::ayu_LocalChannelOwnershipEnabled(tr::now)
				: tr::ayu_LocalChannelOwnershipDisabled(tr::now));
		}, toggle->lifetime());
		Ui::AddDividerText(
			content,
			tr::ayu_LocalChannelOwnershipDescription());
		Ui::AddDividerText(
			content,
			tr::ayu_LocalChannelOwnershipTextOnlyDescription());
		if (!StateFor(channel).posts.empty()) {
			box->addButton(
				tr::ayu_LocalChannelOwnershipClear(),
				[=] {
					controller->show(Ui::MakeConfirmBox({
						.text = tr::ayu_LocalChannelOwnershipClearConfirm(
							tr::now),
						.confirmed = [=](Fn<void()> &&close) {
							ClearBroadcasts(channel);
							controller->showToast(
								tr::ayu_LocalChannelOwnershipCleared(
									tr::now));
							close();
						},
						.confirmText = tr::lng_box_delete(),
						.cancelText = tr::lng_cancel(),
						.confirmStyle = &st::attentionBoxButton,
					}));
				});
		}
		box->addButton(tr::lng_box_done(), [=] {
			box->closeBox();
		});
	}));
}

bool IsLocalChannelBroadcast(HistoryItem *item) {
	return RuntimeUid(item).has_value();
}

void DeleteLocalChannelBroadcast(not_null<HistoryItem*> item) {
	const auto uid = RuntimeUid(item);
	const auto history = item->history();
	const auto channel = history->peer->asBroadcast();
	if (!uid || !channel) {
		return;
	}
	auto state = StateFor(channel);
	state.posts.erase(
		std::remove_if(
			state.posts.begin(),
			state.posts.end(),
			[&](const LocalBroadcast &post) {
				return post.uid == *uid;
			}),
		state.posts.end());
	SaveState(channel, std::move(state));
	auto &runtime = RuntimeBroadcasts()[history];
	runtime.erase(
		std::remove_if(
			runtime.begin(),
			runtime.end(),
			[&](const RuntimeBroadcast &entry) {
				return entry.uid == *uid;
			}),
		runtime.end());
	item->destroy();
	ShowToast(history, tr::ayu_LocalChannelOwnershipDeleted(tr::now));
}

} // namespace Ayu
