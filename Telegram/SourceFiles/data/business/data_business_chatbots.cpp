/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/business/data_business_chatbots.h"

#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "boxes/peers/edit_peer_permissions_box.h"
#include "data/business/data_business_common.h"
#include "data/business/data_business_info.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Data {
namespace {

[[nodiscard]] QJsonObject ReadLocalBusiness(not_null<Main::Session*> session) {
	const auto raw = AyuSettings::localStars(session).localBusiness();
	const auto document = QJsonDocument::fromJson(raw.toUtf8());
	return document.isObject() ? document.object() : QJsonObject();
}

void WriteLocalBusiness(
		not_null<Main::Session*> session,
		const QJsonObject &object) {
	const auto raw = QString::fromUtf8(
		QJsonDocument(object).toJson(QJsonDocument::Compact));
	AyuSettings::localStars(session).setLocalBusiness(raw);
}

[[nodiscard]] QJsonObject BusinessChatsToJson(const BusinessChats &chats) {
	using Type = BusinessChatType;
	auto object = QJsonObject();
	object.insert(u"new"_q, bool(chats.types & Type::NewChats));
	object.insert(u"ex"_q, bool(chats.types & Type::ExistingChats));
	object.insert(u"con"_q, bool(chats.types & Type::Contacts));
	object.insert(u"non"_q, bool(chats.types & Type::NonContacts));
	auto users = QJsonArray();
	for (const auto user : chats.list) {
		users.append(QJsonValue(qint64(user->id.bare)));
	}
	object.insert(u"u"_q, std::move(users));
	return object;
}

[[nodiscard]] BusinessChats BusinessChatsFromJson(
		not_null<Session*> owner,
		const QJsonObject &object) {
	using Type = BusinessChatType;
	auto result = BusinessChats();
	result.types = Type()
		| (object.value(u"new"_q).toBool() ? Type::NewChats : Type())
		| (object.value(u"ex"_q).toBool() ? Type::ExistingChats : Type())
		| (object.value(u"con"_q).toBool() ? Type::Contacts : Type())
		| (object.value(u"non"_q).toBool() ? Type::NonContacts : Type());
	for (const auto &value : object.value(u"u"_q).toArray()) {
		const auto id = value.toVariant().toULongLong();
		if (id) {
			result.list.push_back(owner->user(UserId(id)));
		}
	}
	return result;
}

[[nodiscard]] QJsonObject BusinessRecipientsToJson(
		const BusinessRecipients &data) {
	auto object = QJsonObject();
	object.insert(u"all"_q, data.allButExcluded);
	object.insert(u"inc"_q, BusinessChatsToJson(data.included));
	object.insert(u"exc"_q, BusinessChatsToJson(data.excluded));
	return object;
}

[[nodiscard]] BusinessRecipients BusinessRecipientsFromJson(
		not_null<Session*> owner,
		const QJsonObject &object) {
	auto result = BusinessRecipients();
	result.allButExcluded = object.value(u"all"_q).toBool();
	result.included = BusinessChatsFromJson(
		owner,
		object.value(u"inc"_q).toObject());
	result.excluded = BusinessChatsFromJson(
		owner,
		object.value(u"exc"_q).toObject());
	return result;
}

[[nodiscard]] QJsonObject ChatbotsPermissionsToJson(
		ChatbotsPermissions rights) {
	using Right = ChatbotsPermission;
	auto object = QJsonObject();
	object.insert(u"vm"_q, bool(rights & Right::ViewMessages));
	object.insert(u"rp"_q, bool(rights & Right::ReplyToMessages));
	object.insert(u"mr"_q, bool(rights & Right::MarkAsRead));
	object.insert(u"ds"_q, bool(rights & Right::DeleteSent));
	object.insert(u"dr"_q, bool(rights & Right::DeleteReceived));
	object.insert(u"en"_q, bool(rights & Right::EditName));
	object.insert(u"eb"_q, bool(rights & Right::EditBio));
	object.insert(u"ep"_q, bool(rights & Right::EditUserpic));
	object.insert(u"eu"_q, bool(rights & Right::EditUsername));
	object.insert(u"vg"_q, bool(rights & Right::ViewGifts));
	object.insert(u"sg"_q, bool(rights & Right::SellGifts));
	object.insert(u"gs"_q, bool(rights & Right::GiftSettings));
	object.insert(u"tg"_q, bool(rights & Right::TransferGifts));
	object.insert(u"ts"_q, bool(rights & Right::TransferStars));
	object.insert(u"ms"_q, bool(rights & Right::ManageStories));
	return object;
}

[[nodiscard]] ChatbotsPermissions ChatbotsPermissionsFromJson(
		const QJsonObject &object) {
	using Right = ChatbotsPermission;
	auto result = ChatbotsPermissions();
	const auto add = [&](const QString &key, Right flag) {
		if (object.value(key).toBool()) {
			result |= flag;
		}
	};
	add(u"vm"_q, Right::ViewMessages);
	add(u"rp"_q, Right::ReplyToMessages);
	add(u"mr"_q, Right::MarkAsRead);
	add(u"ds"_q, Right::DeleteSent);
	add(u"dr"_q, Right::DeleteReceived);
	add(u"en"_q, Right::EditName);
	add(u"eb"_q, Right::EditBio);
	add(u"ep"_q, Right::EditUserpic);
	add(u"eu"_q, Right::EditUsername);
	add(u"vg"_q, Right::ViewGifts);
	add(u"sg"_q, Right::SellGifts);
	add(u"gs"_q, Right::GiftSettings);
	add(u"tg"_q, Right::TransferGifts);
	add(u"ts"_q, Right::TransferStars);
	add(u"ms"_q, Right::ManageStories);
	return result;
}

[[nodiscard]] QJsonObject ChatbotsSettingsToJson(
		const ChatbotsSettings &data) {
	auto object = QJsonObject();
	object.insert(u"bot"_q, QJsonValue(
		qint64(data.bot ? data.bot->id.bare : 0)));
	object.insert(u"rec"_q, BusinessRecipientsToJson(data.recipients));
	object.insert(u"perm"_q, ChatbotsPermissionsToJson(data.permissions));
	return object;
}

[[nodiscard]] ChatbotsSettings ChatbotsSettingsFromJson(
		not_null<Session*> owner,
		const QJsonObject &object) {
	auto result = ChatbotsSettings();
	const auto botId = object.value(u"bot"_q).toVariant().toULongLong();
	result.bot = botId ? owner->user(UserId(botId)) : nullptr;
	result.recipients = BusinessRecipientsFromJson(
		owner,
		object.value(u"rec"_q).toObject());
	result.permissions = ChatbotsPermissionsFromJson(
		object.value(u"perm"_q).toObject());
	return result;
}

} // namespace

Chatbots::Chatbots(not_null<Session*> owner)
: _owner(owner) {
}

Chatbots::~Chatbots() = default;

void Chatbots::preload() {
	if (_loaded || _requestId) {
		return;
	}
	const auto session = &_owner->session();
	if (AyuSettings::localStars(session).localPremium()) {
		const auto object = ReadLocalBusiness(session);
		if (object.contains(u"bot"_q)) {
			_settings = ChatbotsSettingsFromJson(
				_owner,
				object.value(u"bot"_q).toObject());
			_loaded = true;
			return;
		}
	}
	_requestId = _owner->session().api().request(
		MTPaccount_GetConnectedBots()
	).done([=](const MTPaccount_ConnectedBots &result) {
		_requestId = 0;
		_loaded = true;

		const auto &data = result.data();
		_owner->processUsers(data.vusers());
		const auto &list = data.vconnected_bots().v;
		if (!list.isEmpty()) {
			const auto &bot = list.front().data();
			const auto botId = bot.vbot_id().v;
			_settings = ChatbotsSettings{
				.bot = _owner->session().data().user(botId),
				.recipients = FromMTP(_owner, bot.vrecipients()),
				.permissions = FromMTP(bot.vrights()),
			};
		} else {
			_settings.force_assign(ChatbotsSettings());
		}
	}).fail([=](const MTP::Error &error) {
		_requestId = 0;
		LOG(("API Error: Could not get connected bots %1 (%2)"
			).arg(error.code()
			).arg(error.type()));
	}).send();
}

bool Chatbots::loaded() const {
	return _loaded;
}

const ChatbotsSettings &Chatbots::current() const {
	return _settings.current();
}

rpl::producer<ChatbotsSettings> Chatbots::changes() const {
	return _settings.changes();
}

rpl::producer<ChatbotsSettings> Chatbots::value() const {
	return _settings.value();
}

void Chatbots::save(
		ChatbotsSettings settings,
		Fn<void()> done,
		Fn<void(QString)> fail) {
	const auto was = _settings.current();
	if (was == settings) {
		return;
	}
	const auto session = &_owner->session();
	if (AyuSettings::localStars(session).localPremium()) {
		auto object = ReadLocalBusiness(session);
		if (settings.bot) {
			object.insert(u"bot"_q, ChatbotsSettingsToJson(settings));
		} else {
			object.remove(u"bot"_q);
		}
		WriteLocalBusiness(session, object);
		_settings = settings;
		if (done) {
			done();
		}
		return;
	} else if (was.bot || settings.bot) {
		using Flag = MTPaccount_UpdateConnectedBot::Flag;
		const auto api = &_owner->session().api();
		api->request(MTPaccount_UpdateConnectedBot(
			MTP_flags(!settings.bot ? Flag::f_deleted : Flag::f_rights),
			ToMTP(settings.permissions),
			(settings.bot ? settings.bot : was.bot)->inputUser(),
			ForBotsToMTP(settings.recipients)
		)).done([=](const MTPUpdates &result) {
			api->applyUpdates(result);
			if (done) {
				done();
			}
		}).fail([=](const MTP::Error &error) {
			_settings = was;
			if (fail) {
				fail(error.type());
			}
		}).send();
	}
	_settings = settings;
}

void Chatbots::togglePaused(not_null<PeerData*> peer, bool paused) {
	const auto type = paused
		? SentRequestType::Pause
		: SentRequestType::Unpause;
	const auto api = &_owner->session().api();
	const auto i = _sentRequests.find(peer);
	if (i != end(_sentRequests)) {
		const auto already = i->second.type;
		if (already == SentRequestType::Remove || already == type) {
			return;
		}
		api->request(i->second.requestId).cancel();
		_sentRequests.erase(i);
	}
	const auto id = api->request(MTPaccount_ToggleConnectedBotPaused(
		peer->input(),
		MTP_bool(paused)
	)).done([=] {
		if (_sentRequests[peer].type != type) {
			return;
		} else if (const auto settings = peer->barSettings()) {
			peer->setBarSettings(paused
				? ((*settings | PeerBarSetting::BusinessBotPaused)
					& ~PeerBarSetting::BusinessBotCanReply)
				: ((*settings & ~PeerBarSetting::BusinessBotPaused)
					| PeerBarSetting::BusinessBotCanReply));
		} else {
			api->requestPeerSettings(peer);
		}
		_sentRequests.remove(peer);
	}).fail([=] {
		if (_sentRequests[peer].type != type) {
			return;
		}
		api->requestPeerSettings(peer);
		_sentRequests.remove(peer);
	}).send();
	_sentRequests[peer] = SentRequest{ type, id };
}

void Chatbots::removeFrom(not_null<PeerData*> peer) {
	const auto session = &_owner->session();
	if (AyuSettings::localStars(session).localPremium()) {
		const auto object = ReadLocalBusiness(session);
		const auto bot = object.value(u"bot"_q).toObject();
		const auto botId = bot.value(u"bot"_q).toVariant().toULongLong();
		if (botId && peer->id.value == botId) {
			auto updated = object;
			updated.remove(u"bot"_q);
			WriteLocalBusiness(session, updated);
			_settings = ChatbotsSettings();
			reload();
			return;
		}
	}
	const auto type = SentRequestType::Remove;
	const auto api = &_owner->session().api();
	const auto i = _sentRequests.find(peer);
	if (i != end(_sentRequests)) {
		const auto already = i->second.type;
		if (already == type) {
			return;
		}
		api->request(i->second.requestId).cancel();
		_sentRequests.erase(i);
	}
	const auto id = api->request(MTPaccount_DisablePeerConnectedBot(
		peer->input()
	)).done([=] {
		if (_sentRequests[peer].type != type) {
			return;
		} else if (const auto settings = peer->barSettings()) {
			peer->clearBusinessBot();
		} else {
			api->requestPeerSettings(peer);
		}
		_sentRequests.remove(peer);
		reload();
	}).fail([=] {
		api->requestPeerSettings(peer);
		_sentRequests.remove(peer);
	}).send();
	_sentRequests[peer] = SentRequest{ type, id };
}

void Chatbots::reload() {
	_loaded = false;
	_owner->session().api().request(base::take(_requestId)).cancel();
	preload();
}

EditFlagsDescriptor<ChatbotsPermissions> ChatbotsPermissionsLabels() {
	using Flag = ChatbotsPermission;

	using PermissionLabel = EditFlagsLabel<ChatbotsPermissions>;
	auto messages = std::vector<PermissionLabel>{
		{ Flag::ViewMessages, tr::lng_chatbots_read(tr::now) },
		{ Flag::ReplyToMessages, tr::lng_chatbots_reply(tr::now) },
		{ Flag::MarkAsRead, tr::lng_chatbots_mark_as_read(tr::now) },
		{ Flag::DeleteSent, tr::lng_chatbots_delete_sent(tr::now) },
		{ Flag::DeleteReceived, tr::lng_chatbots_delete_received(tr::now) },
	};
	auto manage = std::vector<PermissionLabel>{
		{ Flag::EditName, tr::lng_chatbots_edit_name(tr::now) },
		{ Flag::EditBio, tr::lng_chatbots_edit_bio(tr::now) },
		{ Flag::EditUserpic, tr::lng_chatbots_edit_userpic(tr::now) },
		{ Flag::EditUsername, tr::lng_chatbots_edit_username(tr::now) },
	};
	auto gifts = std::vector<PermissionLabel>{
		{ Flag::ViewGifts, tr::lng_chatbots_view_gifts(tr::now) },
		{ Flag::SellGifts, tr::lng_chatbots_sell_gifts(tr::now) },
		{ Flag::GiftSettings, tr::lng_chatbots_gift_settings(tr::now) },
		{ Flag::TransferGifts, tr::lng_chatbots_transfer_gifts(tr::now) },
		{ Flag::TransferStars, tr::lng_chatbots_transfer_stars(tr::now) },
	};
	auto stories = std::vector<PermissionLabel>{
		{ Flag::ManageStories, tr::lng_chatbots_manage_stories(tr::now) },
	};
	return { .labels = {
		{ tr::lng_chatbots_manage_messages(), std::move(messages) },
		{ tr::lng_chatbots_manage_profile(), std::move(manage) },
		{ tr::lng_chatbots_manage_gifts(), std::move(gifts) },
		{ std::nullopt, std::move(stories) },
	}, .st = nullptr };
}

} // namespace Data
