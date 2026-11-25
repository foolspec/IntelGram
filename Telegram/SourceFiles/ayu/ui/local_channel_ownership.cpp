#include "ayu/ui/local_channel_ownership.h"

#include "ayu/data/intelgram_vault.h"
#include "ayu/ui/utils/ayu_profile_values.h"
#include "base/timer.h"
#include "base/unixtime.h"
#include "boxes/peers/edit_peer_info_box.h"
#include "data/data_changes.h"
#include "data/data_channel.h"
#include "data/data_message_reaction_id.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/text/text_entity.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include "styles/style_boxes.h"
#include "styles/style_info.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <algorithm>
#include <array>
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
constexpr auto kMaxViews = 2'000'000'000;
constexpr auto kMaxReactionCount = 1'000'000'000;
constexpr auto kMaxMetricInterval = 24 * 60 * 60;
constexpr auto kMetricTickInterval = crl::time(1000);

struct LocalMetrics {
	int viewStart = 1;
	int viewMaximum = 10'000;
	int viewStep = 1;
	int viewInterval = 30;
	int yellowStar = 0;
	int heart = 0;
	int crying = 0;
	int laughing = 0;
	int eyes = 0;
	int shocked = 0;
	int paidStars = 0;
};

struct LocalBroadcast {
	qint64 uid = 0;
	TimeId date = 0;
	TextWithTags text;
	LocalMetrics metrics;
	int views = 1;
	TimeId lastViewUpdate = 0;
	qint64 replyUid = 0;
	bool pinned = false;
	std::map<QString, int> extraReactions;
};

struct LocalOwnershipState {
	bool enabled = false;
	QString title;
	QString about;
	bool publicChannel = false;
	bool directMessages = false;
	bool autoTranslate = false;
	bool signMessages = false;
	bool reactionsEnabled = true;
	int subscribers = 1;
	int administrators = 1;
	LocalMetrics metrics;
	std::vector<LocalBroadcast> posts;
};

struct RuntimeBroadcast {
	qint64 uid = 0;
	FullMsgId fullId;
};

struct RuntimeGiveaway {
	qint64 uid = 0;
	FullMsgId fullId;
};

[[nodiscard]] int BoundedInt(
		const QJsonObject &object,
		QStringView key,
		int fallback,
		int minimum,
		int maximum) {
	const auto value = object.value(key).toVariant().toLongLong();
	return int(std::clamp<qint64>(
		object.contains(key) ? value : fallback,
		minimum,
		maximum));
}

void NormalizeMetrics(LocalMetrics &metrics) {
	metrics.viewStart = std::clamp(metrics.viewStart, 1, kMaxViews);
	metrics.viewMaximum = std::clamp(
		metrics.viewMaximum,
		metrics.viewStart,
		kMaxViews);
	metrics.viewStep = std::clamp(metrics.viewStep, 1, kMaxViews);
	metrics.viewInterval = std::clamp(
		metrics.viewInterval,
		1,
		kMaxMetricInterval);
	metrics.yellowStar = std::clamp(
		metrics.yellowStar,
		0,
		kMaxReactionCount);
	metrics.heart = std::clamp(metrics.heart, 0, kMaxReactionCount);
	metrics.crying = std::clamp(metrics.crying, 0, kMaxReactionCount);
	metrics.laughing = std::clamp(
		metrics.laughing,
		0,
		kMaxReactionCount);
	metrics.eyes = std::clamp(metrics.eyes, 0, kMaxReactionCount);
	metrics.shocked = std::clamp(
		metrics.shocked,
		0,
		kMaxReactionCount);
	metrics.paidStars = std::clamp(
		metrics.paidStars,
		0,
		kMaxReactionCount);
}

[[nodiscard]] LocalMetrics ParseMetrics(
		const QJsonObject &object,
		LocalMetrics fallback = {}) {
	auto result = LocalMetrics{
		.viewStart = BoundedInt(
			object,
			u"viewStart",
			fallback.viewStart,
			1,
			kMaxViews),
		.viewMaximum = BoundedInt(
			object,
			u"viewMaximum",
			fallback.viewMaximum,
			1,
			kMaxViews),
		.viewStep = BoundedInt(
			object,
			u"viewStep",
			fallback.viewStep,
			1,
			kMaxViews),
		.viewInterval = BoundedInt(
			object,
			u"viewInterval",
			fallback.viewInterval,
			1,
			kMaxMetricInterval),
		.yellowStar = BoundedInt(
			object,
			u"yellowStar",
			fallback.yellowStar,
			0,
			kMaxReactionCount),
		.heart = BoundedInt(
			object,
			u"heart",
			fallback.heart,
			0,
			kMaxReactionCount),
		.crying = BoundedInt(
			object,
			u"crying",
			fallback.crying,
			0,
			kMaxReactionCount),
		.laughing = BoundedInt(
			object,
			u"laughing",
			fallback.laughing,
			0,
			kMaxReactionCount),
		.eyes = BoundedInt(
			object,
			u"eyes",
			fallback.eyes,
			0,
			kMaxReactionCount),
		.shocked = BoundedInt(
			object,
			u"shocked",
			fallback.shocked,
			0,
			kMaxReactionCount),
		.paidStars = BoundedInt(
			object,
			u"paidStars",
			fallback.paidStars,
			0,
			kMaxReactionCount),
	};
	NormalizeMetrics(result);
	return result;
}

[[nodiscard]] QJsonObject WriteMetrics(const LocalMetrics &metrics) {
	return {
		{ u"viewStart"_q, metrics.viewStart },
		{ u"viewMaximum"_q, metrics.viewMaximum },
		{ u"viewStep"_q, metrics.viewStep },
		{ u"viewInterval"_q, metrics.viewInterval },
		{ u"yellowStar"_q, metrics.yellowStar },
		{ u"heart"_q, metrics.heart },
		{ u"crying"_q, metrics.crying },
		{ u"laughing"_q, metrics.laughing },
		{ u"eyes"_q, metrics.eyes },
		{ u"shocked"_q, metrics.shocked },
		{ u"paidStars"_q, metrics.paidStars },
	};
}

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
		.title = object.value(u"title"_q).toString().left(128),
		.about = object.value(u"about"_q).toString().left(4096),
		.publicChannel = object.value(u"publicChannel"_q).toBool(),
		.directMessages = object.value(u"directMessages"_q).toBool(),
		.autoTranslate = object.value(u"autoTranslate"_q).toBool(),
		.signMessages = object.value(u"signMessages"_q).toBool(),
		.reactionsEnabled = object.value(
			u"reactionsEnabled"_q
		).toBool(true),
		.subscribers = BoundedInt(
			object,
			u"subscribers",
			1,
			1,
			kMaxViews),
		.administrators = BoundedInt(
			object,
			u"administrators",
			1,
			1,
			kMaxViews),
		.metrics = ParseMetrics(object.value(u"metrics"_q).toObject()),
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
			.metrics = ParseMetrics(
				post.value(u"metrics"_q).toObject(),
				result.metrics),
			.views = BoundedInt(
				post,
				u"views",
				result.metrics.viewStart,
				1,
				kMaxViews),
			.lastViewUpdate = TimeId(BoundedInt(
				post,
				u"lastViewUpdate",
				post.value(u"date"_q).toInt(),
				1,
				kMaxViews)),
			.replyUid = post.value(u"replyUid"_q).toString().toLongLong(),
			.pinned = post.value(u"pinned"_q).toBool(),
		};
		const auto extraReactions = post.value(
			u"extraReactions"_q
		).toObject();
		for (auto i = extraReactions.begin(); i != extraReactions.end(); ++i) {
			const auto key = i.key().left(128);
			const auto count = int(std::clamp<qint64>(
				i.value().toVariant().toLongLong(),
				0,
				kMaxReactionCount));
			if (!key.isEmpty() && count > 0) {
				parsed.extraReactions.emplace(key, count);
			}
		}
		parsed.views = std::clamp(
			parsed.views,
			parsed.metrics.viewStart,
			parsed.metrics.viewMaximum);
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
		object.insert(u"metrics"_q, WriteMetrics(post.metrics));
		object.insert(u"views"_q, post.views);
		object.insert(u"lastViewUpdate"_q, int(post.lastViewUpdate));
		object.insert(u"replyUid"_q, QString::number(post.replyUid));
		object.insert(u"pinned"_q, post.pinned);
		auto extraReactions = QJsonObject();
		for (const auto &[key, count] : post.extraReactions) {
			if (!key.isEmpty() && count > 0) {
				extraReactions.insert(
					key.left(128),
					std::clamp(count, 0, kMaxReactionCount));
			}
		}
		object.insert(u"extraReactions"_q, extraReactions);
		posts.push_back(object);
	}
	return QJsonDocument(QJsonObject{
		{ u"version"_q, 3 },
		{ u"enabled"_q, state.enabled },
		{ u"title"_q, state.title.left(128) },
		{ u"about"_q, state.about.left(4096) },
		{ u"publicChannel"_q, state.publicChannel },
		{ u"directMessages"_q, state.directMessages },
		{ u"autoTranslate"_q, state.autoTranslate },
		{ u"signMessages"_q, state.signMessages },
		{ u"reactionsEnabled"_q, state.reactionsEnabled },
		{ u"subscribers"_q, state.subscribers },
		{ u"administrators"_q, state.administrators },
		{ u"metrics"_q, WriteMetrics(state.metrics) },
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
	NormalizeMetrics(state.metrics);
	state.subscribers = std::clamp(state.subscribers, 1, kMaxViews);
	state.administrators = std::clamp(
		state.administrators,
		1,
		kMaxViews);
	for (auto &post : state.posts) {
		NormalizeMetrics(post.metrics);
		post.views = std::clamp(
			post.views,
			post.metrics.viewStart,
			post.metrics.viewMaximum);
		for (auto i = post.extraReactions.begin()
				; i != post.extraReactions.end();) {
			i->second = std::clamp(i->second, 0, kMaxReactionCount);
			if (i->first.isEmpty() || i->second <= 0) {
				i = post.extraReactions.erase(i);
			} else {
				++i;
			}
		}
	}
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

[[nodiscard]] std::map<History*, std::vector<RuntimeGiveaway>>
		&RuntimeGiveaways() {
	static auto result
		= std::map<History*, std::vector<RuntimeGiveaway>>();
	return result;
}

void CleanRuntimeGiveaways(not_null<History*> history) {
	const auto i = RuntimeGiveaways().find(history);
	if (i == RuntimeGiveaways().end()) {
		return;
	}
	auto &runtime = i->second;
	runtime.erase(
		std::remove_if(
			runtime.begin(),
			runtime.end(),
			[=](const RuntimeGiveaway &entry) {
				return !history->owner().message(entry.fullId);
			}),
		runtime.end());
}

[[nodiscard]] LocalBroadcast *FindPost(
		LocalOwnershipState &state,
		qint64 uid) {
	const auto i = std::find_if(
		state.posts.begin(),
		state.posts.end(),
		[=](const LocalBroadcast &post) {
			return post.uid == uid;
		});
	return (i == state.posts.end()) ? nullptr : &*i;
}

[[nodiscard]] bool AdvanceViews(
		LocalBroadcast &post,
		TimeId now) {
	if (post.views >= post.metrics.viewMaximum) {
		return false;
	}
	if (post.lastViewUpdate <= 0) {
		post.lastViewUpdate = now;
		return true;
	}
	const auto elapsed = std::max<TimeId>(0, now - post.lastViewUpdate);
	const auto ticks = elapsed / post.metrics.viewInterval;
	if (!ticks) {
		return false;
	}
	const auto increase = qint64(ticks) * post.metrics.viewStep;
	post.views = int(std::min<qint64>(
		post.metrics.viewMaximum,
		qint64(post.views) + increase));
	post.lastViewUpdate += ticks * post.metrics.viewInterval;
	return true;
}

void ApplyMetrics(
		not_null<HistoryItem*> item,
		const LocalBroadcast &post,
		bool reactionsEnabled) {
	if (item->changeViewsCount(post.views)) {
		item->history()->owner().notifyItemDataChange(item);
	}

	auto results = QVector<MTPReactionCount>();
	const auto addEmoji = [&](const char *emoji, int count) {
		if (!reactionsEnabled || count <= 0) {
			return;
		}
		results.push_back(MTP_reactionCount(
			MTP_flags(MTPDreactionCount::Flag(0)),
			MTP_int(0),
			MTP_reactionEmoji(MTP_string(QString::fromUtf8(emoji))),
			MTP_int(count)));
	};
	addEmoji("\xE2\xAD\x90", post.metrics.yellowStar);
	addEmoji("\xE2\x9D\xA4", post.metrics.heart);
	addEmoji("\xF0\x9F\x98\xAD", post.metrics.crying);
	addEmoji("\xF0\x9F\xA4\xA3", post.metrics.laughing);
	addEmoji("\xF0\x9F\x91\x80", post.metrics.eyes);
	addEmoji("\xF0\x9F\x98\xB1", post.metrics.shocked);
	for (const auto &[key, count] : post.extraReactions) {
		if (!reactionsEnabled || count <= 0) {
			continue;
		}
		const auto customPrefix = u"custom:"_q;
		if (key.startsWith(customPrefix)) {
			const auto id = key.mid(customPrefix.size()).toULongLong();
			if (id) {
				results.push_back(MTP_reactionCount(
					MTP_flags(MTPDreactionCount::Flag(0)),
					MTP_int(0),
					MTP_reactionCustomEmoji(MTP_long(id)),
					MTP_int(count)));
			}
		} else {
			addEmoji(key.toUtf8().constData(), count);
		}
	}
	if (reactionsEnabled && post.metrics.paidStars > 0) {
		results.push_back(MTP_reactionCount(
			MTP_flags(MTPDreactionCount::Flag(0)),
			MTP_int(0),
			MTP_reactionPaid(),
			MTP_int(post.metrics.paidStars)));
	}
	if (results.isEmpty()) {
		item->updateReactions(nullptr);
		return;
	}
	const auto reactions = MTPMessageReactions(MTP_messageReactions(
		MTP_flags(MTPDmessageReactions::Flag(0)),
		MTP_vector<MTPReactionCount>(results),
		MTP_vector<MTPMessagePeerReaction>(),
		MTP_vector<MTPMessageReactor>()));
	item->updateReactions(&reactions);
}

void UpdateRuntimeMetrics() {
	const auto now = base::unixtime::now();
	for (const auto &[history, entries] : RuntimeBroadcasts()) {
		const auto channel = history->peer->asBroadcast();
		if (!channel) {
			continue;
		}
		auto state = StateFor(channel);
		auto changed = false;
		for (const auto &entry : entries) {
			const auto post = FindPost(state, entry.uid);
			const auto item = history->owner().message(entry.fullId);
			if (!post || !item || !AdvanceViews(*post, now)) {
				continue;
			}
			ApplyMetrics(item, *post, state.reactionsEnabled);
			changed = true;
		}
		if (changed) {
			SaveState(channel, std::move(state));
		}
	}
}

[[nodiscard]] base::Timer &MetricsTimer() {
	static auto result = base::Timer([] {
		UpdateRuntimeMetrics();
	});
	return result;
}

void EnsureMetricsTimer() {
	auto &timer = MetricsTimer();
	if (!timer.isActive()) {
		timer.callEach(kMetricTickInterval);
	}
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
	auto replyTo = FullReplyTo();
	if (post.replyUid) {
		const auto &runtime = RuntimeBroadcasts()[history];
		const auto i = std::find_if(
			runtime.begin(),
			runtime.end(),
			[&](const RuntimeBroadcast &entry) {
				return entry.uid == post.replyUid;
			});
		if (i != runtime.end()) {
			replyTo.messageId = i->fullId;
		}
	}
	const auto item = history->addNewLocalMessage({
		.id = history->owner().nextLocalMessageId(),
		.flags = MessageFlag::Post
			| MessageFlag::HasViews
			| (post.pinned ? MessageFlag::Pinned : MessageFlag())
			| (replyTo ? MessageFlag::HasReplyInfo : MessageFlag()),
		.replyTo = replyTo,
		.date = post.date,
	}, text, MTP_messageMediaEmpty());
	RuntimeBroadcasts()[history].push_back({
		.uid = post.uid,
		.fullId = item->fullId(),
	});
	if (post.pinned) {
		item->setIsPinned(true);
	}
	const auto channel = history->peer->asBroadcast();
	ApplyMetrics(
		item,
		post,
		!channel || StateFor(channel).reactionsEnabled);
	EnsureMetricsTimer();
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

[[nodiscard]] not_null<Ui::SettingsButton*> AddOwnerRow(
		not_null<Ui::VerticalLayout*> parent,
		rpl::producer<QString> text,
		rpl::producer<QString> label,
		Fn<void()> callback,
		const style::icon *icon) {
	return parent->add(EditPeerInfoBox::CreateButton(
		parent,
		std::move(text),
		std::move(label),
		std::move(callback),
		st::manageGroupTopButtonWithText,
		{ icon }));
}

[[nodiscard]] not_null<Ui::InputField*> AddNumberField(
		not_null<Ui::GenericBox*> box,
		const QString &placeholder,
		int value) {
	return box->addRow(
		object_ptr<Ui::InputField>(
			box,
			st::defaultInputField,
			rpl::single(placeholder),
			QString::number(value)),
		st::boxRowPadding);
}

[[nodiscard]] std::optional<int> ReadNumberField(
		not_null<Ui::InputField*> field,
		int minimum,
		int maximum) {
	auto ok = false;
	const auto parsed = field->getLastText().trimmed().toLongLong(&ok);
	if (!ok || parsed < minimum || parsed > maximum) {
		field->showError();
		return std::nullopt;
	}
	return int(parsed);
}

void RefreshAllRuntimeMetrics(
		not_null<ChannelData*> channel,
		const LocalOwnershipState &state) {
	const auto history = channel->owner().history(channel);
	CleanRuntimeBroadcasts(history);
	const auto i = RuntimeBroadcasts().find(history);
	if (i == RuntimeBroadcasts().end()) {
		return;
	}
	for (const auto &entry : i->second) {
		const auto item = history->owner().message(entry.fullId);
		const auto post = std::find_if(
			state.posts.begin(),
			state.posts.end(),
			[=](const LocalBroadcast &post) {
				return post.uid == entry.uid;
			});
		if (item && post != state.posts.end()) {
			ApplyMetrics(item, *post, state.reactionsEnabled);
		}
	}
}

void ShowLocalMetricsEditor(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel) {
	const auto saved = StateFor(channel);
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setWidth(st::boxWideWidth);
		box->setTitle(rpl::single(u"Broadcast engagement"_q));
		Ui::AddSubsectionTitle(
			box->verticalLayout(),
			rpl::single(u"Incremental views"_q));
		const auto viewStart = AddNumberField(
			box,
			u"Starting views"_q,
			saved.metrics.viewStart);
		const auto viewMaximum = AddNumberField(
			box,
			u"Maximum views"_q,
			saved.metrics.viewMaximum);
		const auto viewStep = AddNumberField(
			box,
			u"Views added each interval"_q,
			saved.metrics.viewStep);
		const auto viewInterval = AddNumberField(
			box,
			u"Interval in seconds"_q,
			saved.metrics.viewInterval);

		Ui::AddSubsectionTitle(
			box->verticalLayout(),
			rpl::single(u"Local reactions and Stars"_q));
		const auto yellowStar = AddNumberField(
			box,
			u"Star reaction count"_q,
			saved.metrics.yellowStar);
		const auto heart = AddNumberField(
			box,
			u"Heart reaction count"_q,
			saved.metrics.heart);
		const auto crying = AddNumberField(
			box,
			u"Crying reaction count"_q,
			saved.metrics.crying);
		const auto laughing = AddNumberField(
			box,
			u"Laughing reaction count"_q,
			saved.metrics.laughing);
		const auto eyes = AddNumberField(
			box,
			u"Eyes reaction count"_q,
			saved.metrics.eyes);
		const auto shocked = AddNumberField(
			box,
			u"Shocked reaction count"_q,
			saved.metrics.shocked);
		const auto paidStars = AddNumberField(
			box,
			u"Paid Stars count"_q,
			saved.metrics.paidStars);
		Ui::AddDividerText(
			box->verticalLayout(),
			rpl::single(
				u"These counters are simulated only for local broadcasts "
				"inside this IntelGram client. They never contact Telegram."_q));

		box->setFocusCallback([=] {
			viewStart->setFocusFast();
		});
		box->addButton(rpl::single(u"Apply to broadcasts"_q), [=] {
			const auto start = ReadNumberField(
				viewStart,
				1,
				kMaxViews);
			const auto maximum = ReadNumberField(
				viewMaximum,
				1,
				kMaxViews);
			const auto step = ReadNumberField(
				viewStep,
				1,
				kMaxViews);
			const auto interval = ReadNumberField(
				viewInterval,
				1,
				kMaxMetricInterval);
			const auto star = ReadNumberField(
				yellowStar,
				0,
				kMaxReactionCount);
			const auto heartValue = ReadNumberField(
				heart,
				0,
				kMaxReactionCount);
			const auto cryingValue = ReadNumberField(
				crying,
				0,
				kMaxReactionCount);
			const auto laughingValue = ReadNumberField(
				laughing,
				0,
				kMaxReactionCount);
			const auto eyesValue = ReadNumberField(
				eyes,
				0,
				kMaxReactionCount);
			const auto shockedValue = ReadNumberField(
				shocked,
				0,
				kMaxReactionCount);
			const auto paidValue = ReadNumberField(
				paidStars,
				0,
				kMaxReactionCount);
			if (!start
				|| !maximum
				|| !step
				|| !interval
				|| !star
				|| !heartValue
				|| !cryingValue
				|| !laughingValue
				|| !eyesValue
				|| !shockedValue
				|| !paidValue) {
				return;
			} else if (*maximum < *start) {
				viewMaximum->showError();
				return;
			}
			auto state = StateFor(channel);
			state.metrics = {
				.viewStart = *start,
				.viewMaximum = *maximum,
				.viewStep = *step,
				.viewInterval = *interval,
				.yellowStar = *star,
				.heart = *heartValue,
				.crying = *cryingValue,
				.laughing = *laughingValue,
				.eyes = *eyesValue,
				.shocked = *shockedValue,
				.paidStars = *paidValue,
			};
			const auto now = base::unixtime::now();
			for (auto &post : state.posts) {
				post.metrics = state.metrics;
				post.views = state.metrics.viewStart;
				post.lastViewUpdate = now;
			}
			SaveState(channel, state);
			const auto history = channel->owner().history(channel);
			RemoveRuntimeBroadcasts(history);
			EnsureLocalChannelBroadcasts(history);
			EnsureMetricsTimer();
			controller->showToast(
				u"Local broadcast engagement updated."_q);
			box->closeBox();
		});
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
}

void ShowLocalAudienceEditor(
		not_null<Window::SessionController*> controller,
		not_null<ChannelData*> channel) {
	const auto saved = StateFor(channel);
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(u"Channel audience"_q));
		const auto subscribers = AddNumberField(
			box,
			u"Subscribers"_q,
			saved.subscribers);
		const auto administrators = AddNumberField(
			box,
			u"Administrators"_q,
			saved.administrators);
		Ui::AddDividerText(
			box->verticalLayout(),
			rpl::single(
				u"Audience counts are part of the local owner preview only."_q));
		box->addButton(tr::lng_settings_save(), [=] {
			const auto subscriberCount = ReadNumberField(
				subscribers,
				1,
				kMaxViews);
			const auto administratorCount = ReadNumberField(
				administrators,
				1,
				kMaxViews);
			if (!subscriberCount || !administratorCount) {
				return;
			}
			auto state = StateFor(channel);
			state.subscribers = *subscriberCount;
			state.administrators = *administratorCount;
			SaveState(channel, std::move(state));
			box->closeBox();
		});
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
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

void AddLocalGiveawayMessage(
		not_null<History*> history,
		const LocalGiveawayRecord &record) {
	const auto session = &history->session();
	auto channels = std::vector<not_null<ChannelData*>>();
	for (const auto id : record.channels) {
		channels.push_back(session->data().channel(ChannelId(id)));
	}
	if (channels.empty()) {
		channels.push_back(session->data().channel(
			ChannelId(record.channelId)));
	}
	auto flags = MessageFlag();
	if (history->peer->isBroadcast()) {
		flags |= MessageFlag::Post | MessageFlag::HasViews;
	}
	const auto item = history->addNewLocalMessage({
		.id = history->owner().nextLocalMessageId(),
		.flags = flags,
		.date = TimeId(record.date),
	}, TextWithEntities(), MTP_messageMediaEmpty());
	item->overrideMedia(std::make_unique<Data::MediaGiveawayStart>(
		item,
		Data::GiveawayStart{
			.channels = std::move(channels),
			.countries = record.countries,
			.additionalPrize = record.prize,
			.untilDate = TimeId(record.untilDate),
			.quantity = record.quantity,
			.months = record.months,
			.credits = uint64(record.credits),
			.all = !record.onlyNewSubscribers,
		}));
	RuntimeGiveaways()[history].push_back({
		.uid = record.uid,
		.fullId = item->fullId(),
	});
	history->session().changes().historyUpdated(
		history,
		Data::HistoryUpdate::Flag::MessageSent);
}

void EnsureLocalGiveaways(not_null<History*> history) {
	const auto channel = history->peer->asChannel();
	if (!channel) {
		return;
	}
	const auto channelId = peerToChannel(channel->id).bare;
	const auto session = &history->session();
	CleanRuntimeGiveaways(history);
	auto &runtime = RuntimeGiveaways()[history];
	for (const auto &record : LocalGiveaways(session)) {
		if (record.channelId != channelId) {
			continue;
		}
		const auto exists = std::any_of(
			runtime.begin(),
			runtime.end(),
			[&](const RuntimeGiveaway &entry) {
				return entry.uid == record.uid;
			});
		if (!exists) {
			AddLocalGiveawayMessage(history, record);
		}
	}
}

bool IsLocalGiveaway(HistoryItem *item) {
	if (!item) {
		return false;
	}
	const auto history = item->history();
	CleanRuntimeGiveaways(history);
	const auto i = RuntimeGiveaways().find(history);
	if (i == RuntimeGiveaways().end()) {
		return false;
	}
	const auto fullId = item->fullId();
	return std::any_of(
		i->second.begin(),
		i->second.end(),
		[&](const RuntimeGiveaway &entry) {
			return entry.fullId == fullId;
		});
}

LocalChannelBroadcastResult HandleLocalChannelBroadcast(
		not_null<History*> history,
		const TextWithTags &text,
		bool scheduled,
		HistoryItem *replyTo) {
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
		.metrics = state.metrics,
		.views = state.metrics.viewStart,
		.lastViewUpdate = base::unixtime::now(),
		.replyUid = (replyTo && replyTo->history() == history)
			? RuntimeUid(replyTo).value_or(0)
			: 0,
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
		const auto saved = StateFor(channel);
		if (!saved.enabled) {
			box->setTitle(tr::ayu_LocalChannelOwnershipTitle());
			const auto content = box->verticalLayout();
			const auto toggle = content->add(
				object_ptr<Ui::SettingsButton>(
					content,
					tr::ayu_LocalChannelOwnershipToggle(),
					st::settingsButtonNoIcon));
			toggle->toggleOn(rpl::single(false));
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
			box->addButton(tr::lng_box_done(), [=] {
				box->closeBox();
			});
			return;
		}

		box->setTitle(tr::lng_edit_channel_title());
		const auto content = box->verticalLayout();
		const auto title = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::lng_dlg_new_channel_name(),
				saved.title.isEmpty() ? channel->name() : saved.title),
			st::boxRowPadding);
		title->setMaxLength(128);
		const auto about = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				Ui::InputField::Mode::MultiLine,
				tr::lng_create_group_description(),
				saved.about.isEmpty() ? channel->about() : saved.about),
			st::boxRowPadding);
		about->setMaxLength(4096);

		struct EditorState {
			bool publicChannel = false;
			bool directMessages = false;
			bool autoTranslate = false;
			bool signMessages = false;
			bool reactionsEnabled = true;
			rpl::event_stream<> labelsChanged;
		};
		const auto editor = box->lifetime().make_state<EditorState>();
		editor->publicChannel = saved.publicChannel;
		editor->directMessages = saved.directMessages;
		editor->autoTranslate = saved.autoTranslate;
		editor->signMessages = saved.signMessages;
		editor->reactionsEnabled = saved.reactionsEnabled;

		Ui::AddSkip(content);
		Ui::AddDivider(content);
		Ui::AddSkip(content);
		AddOwnerRow(
			content,
			tr::lng_manage_peer_channel_type(),
			editor->labelsChanged.events_starting_with(
				{}
			) | rpl::map([=] {
				return editor->publicChannel
					? u"Public"_q
					: u"Private"_q;
			}),
			[=] {
				editor->publicChannel = !editor->publicChannel;
				editor->labelsChanged.fire({});
			},
			&st::menuIconCustomize);
		AddOwnerRow(
			content,
			tr::lng_manage_discussion_group(),
			tr::lng_manage_discussion_group_add(),
			[=] {
				controller->showToast(
					u"Discussion settings are kept in this local preview."_q);
			},
			&st::menuIconGroups);
		const auto directMessages = AddOwnerRow(
			content,
			tr::lng_manage_monoforum(),
			rpl::single(QString()),
			[] {},
			&st::menuIconChats);
		directMessages->toggleOn(rpl::single(saved.directMessages));
		directMessages->toggledValue(
		) | rpl::on_next([=](bool value) {
			editor->directMessages = value;
		}, directMessages->lifetime());
		AddOwnerRow(
			content,
			rpl::single(u"Appearance"_q),
			rpl::single(u"Default"_q),
			[=] {
				controller->showToast(
					u"Channel appearance is local to this IntelGram view."_q);
			},
			&st::menuIconCustomize);
		const auto autoTranslate = AddOwnerRow(
			content,
			tr::lng_edit_autotranslate(),
			rpl::single(QString()),
			[] {},
			&st::menuIconTranslate);
		autoTranslate->toggleOn(rpl::single(saved.autoTranslate));
		autoTranslate->toggledValue(
		) | rpl::on_next([=](bool value) {
			editor->autoTranslate = value;
		}, autoTranslate->lifetime());
		const auto signMessages = AddOwnerRow(
			content,
			tr::lng_edit_sign_messages(),
			rpl::single(QString()),
			[] {},
			&st::menuIconSigned);
		signMessages->toggleOn(rpl::single(saved.signMessages));
		signMessages->toggledValue(
		) | rpl::on_next([=](bool value) {
			editor->signMessages = value;
		}, signMessages->lifetime());

		Ui::AddDividerText(
			content,
			rpl::single(
				u"Owner controls on this page affect only the local channel "
				"preview and local broadcasts."_q));
		const auto reactions = AddOwnerRow(
			content,
			tr::lng_manage_peer_reactions(),
			editor->labelsChanged.events_starting_with(
				{}
			) | rpl::map([=] {
				return editor->reactionsEnabled ? u"On"_q : u"Off"_q;
			}),
			[] {},
			&st::menuIconGroupReactions);
		reactions->toggleOn(rpl::single(saved.reactionsEnabled));
		reactions->toggledValue(
		) | rpl::on_next([=](bool value) {
			editor->reactionsEnabled = value;
			editor->labelsChanged.fire({});
		}, reactions->lifetime());
		AddOwnerRow(
			content,
			rpl::single(u"Broadcast engagement"_q),
			rpl::single(
				QString::number(saved.metrics.viewMaximum)
					+ u" max"_q),
			[=] {
				ShowLocalMetricsEditor(controller, channel);
			},
			&st::menuIconStats);
		AddOwnerRow(
			content,
			tr::lng_manage_peer_invite_links(),
			rpl::single(u"1"_q),
			[=] {
				controller->showToast(
					u"Local preview invite link copied."_q);
			},
			&st::menuIconLinks);
		AddOwnerRow(
			content,
			tr::lng_manage_peer_administrators(),
			rpl::single(QString::number(saved.administrators)),
			[=] {
				ShowLocalAudienceEditor(controller, channel);
			},
			&st::menuIconAdmin);
		AddOwnerRow(
			content,
			tr::lng_manage_peer_subscribers(),
			rpl::single(QString::number(saved.subscribers)),
			[=] {
				ShowLocalAudienceEditor(controller, channel);
			},
			&st::menuIconGroups);
		AddOwnerRow(
			content,
			tr::lng_manage_peer_removed_users(),
			rpl::single(QString()),
			[=] {
				controller->showToast(u"No locally removed users."_q);
			},
			&st::menuIconRemove);
		AddOwnerRow(
			content,
			tr::lng_manage_peer_recent_actions(),
			rpl::single(QString()),
			[=] {
				controller->showToast(
					u"Local ownership enabled for this channel."_q);
			},
			&st::menuIconGroupLog);
		AddOwnerRow(
			content,
			tr::lng_manage_peer_star_ref(),
			rpl::single(QString()),
			[=] {
				controller->showToast(
					u"Affiliate settings remain local."_q);
			},
			&st::menuIconStarRefShare);

		Ui::AddDividerText(
			content,
			tr::ayu_LocalChannelOwnershipTextOnlyDescription());
		content->add(EditPeerInfoBox::CreateButton(
			content,
			tr::lng_profile_delete_channel(),
			rpl::single(QString()),
			[=] {
				controller->show(Ui::MakeConfirmBox({
					.text = rpl::single(
						u"Remove the local owner preview and all of its "
						"local broadcasts?"_q),
					.confirmed = [=](Fn<void()> close) {
						ClearBroadcasts(channel);
						SetOwnershipEnabled(channel, false);
						close();
						box->closeBox();
						controller->showToast(
							tr::ayu_LocalChannelOwnershipDisabled(tr::now));
					},
					.confirmText = tr::lng_box_delete(),
					.cancelText = tr::lng_cancel(),
					.confirmStyle = &st::attentionBoxButton,
				}));
			},
			st::manageDeleteGroupButton,
			{}));

		box->setFocusCallback([=] {
			title->setFocusFast();
		});
		box->addButton(tr::lng_settings_save(), [=] {
			auto state = StateFor(channel);
			state.title = title->getLastText().trimmed().left(128);
			state.about = about->getLastText().trimmed().left(4096);
			state.publicChannel = editor->publicChannel;
			state.directMessages = editor->directMessages;
			state.autoTranslate = editor->autoTranslate;
			state.signMessages = editor->signMessages;
			state.reactionsEnabled = editor->reactionsEnabled;
			SaveState(channel, state);
			RefreshAllRuntimeMetrics(channel, state);
			controller->showToast(u"Local channel settings saved."_q);
			box->closeBox();
		});
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
}

bool IsLocalChannelBroadcast(HistoryItem *item) {
	return RuntimeUid(item).has_value();
}

void RefreshLocalChannelBroadcastMetrics(HistoryItem *item) {
	const auto uid = RuntimeUid(item);
	const auto channel = item
		? item->history()->peer->asBroadcast()
		: nullptr;
	if (!uid || !channel) {
		return;
	}
	auto state = StateFor(channel);
	const auto post = FindPost(state, *uid);
	if (!post) {
		return;
	}
	const auto changed = AdvanceViews(*post, base::unixtime::now());
	ApplyMetrics(item, *post, state.reactionsEnabled);
	if (changed) {
		SaveState(channel, std::move(state));
	}
}

bool LocalChannelBroadcastPinned(HistoryItem *item) {
	const auto uid = RuntimeUid(item);
	const auto channel = item
		? item->history()->peer->asBroadcast()
		: nullptr;
	if (!uid || !channel) {
		return false;
	}
	auto state = StateFor(channel);
	const auto post = FindPost(state, *uid);
	return post && post->pinned;
}

void ToggleLocalChannelBroadcastPinned(not_null<HistoryItem*> item) {
	const auto uid = RuntimeUid(item);
	const auto history = item->history();
	const auto channel = history->peer->asBroadcast();
	if (!uid || !channel) {
		return;
	}
	auto state = StateFor(channel);
	const auto post = FindPost(state, *uid);
	if (!post) {
		return;
	}
	post->pinned = !post->pinned;
	const auto pinned = post->pinned;
	SaveState(channel, std::move(state));
	item->setIsPinned(pinned);
	ShowToast(
		history,
		pinned
			? tr::lng_pinned_message(tr::now)
			: tr::lng_context_unpin_msg(tr::now));
}

void ShowEditLocalChannelBroadcast(
		not_null<Window::SessionController*> controller,
		not_null<HistoryItem*> item) {
	const auto uid = RuntimeUid(item);
	const auto history = item->history();
	const auto channel = history->peer->asBroadcast();
	if (!uid || !channel) {
		return;
	}
	auto state = StateFor(channel);
	const auto saved = FindPost(state, *uid);
	if (!saved) {
		return;
	}
	const auto initial = saved->text.text;
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::lng_edit_message());
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(
				box,
				st::defaultInputField,
				tr::lng_edit_message_text(),
				initial),
			st::boxRowPadding);
		field->setMaxLength(kMaxPostLength);
		const auto save = [=] {
			const auto text = field->getLastText().trimmed();
			if (text.isEmpty()) {
				field->showError();
				return;
			}
			auto changed = StateFor(channel);
			const auto post = FindPost(changed, *uid);
			if (!post) {
				box->closeBox();
				return;
			}
			post->text = TextWithTags{ .text = text.left(kMaxPostLength) };
			SaveState(channel, std::move(changed));
			RemoveRuntimeBroadcasts(history);
			EnsureLocalChannelBroadcasts(history);
			history->session().changes().historyUpdated(
				history,
				Data::HistoryUpdate::Flag::MessageSent);
			box->closeBox();
		};
		field->submits(
		) | rpl::on_next(save, field->lifetime());
		box->setFocusCallback([=] {
			field->setFocusFast();
		});
		box->addButton(tr::lng_settings_save(), save);
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
}

void ConfirmDeleteLocalChannelBroadcast(
		not_null<Window::SessionController*> controller,
		not_null<HistoryItem*> item) {
	const auto history = item->history();
	const auto uid = RuntimeUid(item);
	if (!uid) {
		return;
	}
	controller->show(Ui::MakeConfirmBox({
		.text = tr::ayu_LocalChannelOwnershipDelete(),
		.confirmed = [=](Fn<void()> close) {
			const auto current = RuntimeBroadcasts().find(history);
			if (current != RuntimeBroadcasts().end()) {
				const auto entry = std::find_if(
					current->second.begin(),
					current->second.end(),
					[=](const RuntimeBroadcast &entry) {
						return entry.uid == *uid;
					});
				if (entry != current->second.end()) {
					if (const auto post = history->owner().message(
							entry->fullId)) {
						DeleteLocalChannelBroadcast(post);
					}
				}
			}
			close();
		},
		.confirmText = tr::lng_box_delete(),
		.cancelText = tr::lng_cancel(),
		.confirmStyle = &st::attentionBoxButton,
	}));
}

QString LocalChannelBroadcastLink(HistoryItem *item) {
	const auto uid = RuntimeUid(item);
	const auto channel = item
		? item->history()->peer->asBroadcast()
		: nullptr;
	if (!uid || !channel) {
		return {};
	}
	return u"intelgram://local-channel/%1/%2/%3"_q
		.arg(QString::number(channel->session().userId().bare))
		.arg(QString::number(channel->id.value))
		.arg(QString::number(*uid));
}

bool IncrementLocalChannelBroadcastReaction(
		not_null<HistoryItem*> item,
		const Data::ReactionId &reaction) {
	const auto uid = RuntimeUid(item);
	const auto channel = item->history()->peer->asBroadcast();
	if (!uid || !channel) {
		return false;
	}
	auto state = StateFor(channel);
	const auto post = FindPost(state, *uid);
	if (!post) {
		return false;
	}
	auto target = (int*)nullptr;
	auto emoji = reaction.emoji();
	if (reaction.paid()) {
		target = &post->metrics.paidStars;
	} else {
		const auto heart = QString::fromUtf8("\xE2\x9D\xA4");
		if (emoji == QString::fromUtf8("\xE2\x9D\xA4\xEF\xB8\x8F")) {
			emoji = heart;
		}
		const auto known = std::array{
			std::pair(
				QString::fromUtf8("\xE2\xAD\x90"),
				&LocalMetrics::yellowStar),
			std::pair(heart, &LocalMetrics::heart),
			std::pair(
				QString::fromUtf8("\xF0\x9F\x98\xAD"),
				&LocalMetrics::crying),
			std::pair(
				QString::fromUtf8("\xF0\x9F\xA4\xA3"),
				&LocalMetrics::laughing),
			std::pair(
				QString::fromUtf8("\xF0\x9F\x91\x80"),
				&LocalMetrics::eyes),
			std::pair(
				QString::fromUtf8("\xF0\x9F\x98\xB1"),
				&LocalMetrics::shocked),
		};
		const auto i = std::find_if(
			known.begin(),
			known.end(),
			[&](const auto &entry) {
				return entry.first == emoji;
			});
		if (i != known.end()) {
			target = &(post->metrics.*i->second);
		}
	}
	if (!target) {
		const auto key = reaction.custom()
			? u"custom:%1"_q.arg(QString::number(reaction.custom()))
			: emoji.left(128);
		if (key.isEmpty()) {
			return false;
		}
		target = &post->extraReactions[key];
	}
	*target = std::min(*target + 1, kMaxReactionCount);
	ApplyMetrics(item, *post, state.reactionsEnabled);
	SaveState(channel, std::move(state));
	return true;
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
