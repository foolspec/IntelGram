/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/business/data_business_info.h"

#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "base/unixtime.h"
#include "data/business/data_business_common.h"
#include "data/business/data_shortcut_messages.h"
#include "data/data_document.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "main/main_session.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Data {
namespace {

constexpr auto kMaxLocalShortcutMessages = 100;

[[nodiscard]] MTPBusinessWorkHours ToMTP(const WorkingHours &data) {
	const auto list = data.intervals.normalized().list;
	const auto proj = [](const WorkingInterval &data) {
		return MTPBusinessWeeklyOpen(MTP_businessWeeklyOpen(
			MTP_int(data.start / 60),
			MTP_int(data.end / 60)));
	};
	return MTP_businessWorkHours(
		MTP_flags(0),
		MTP_string(data.timezoneId),
		MTP_vector_from_range(list | ranges::views::transform(proj)));
}

[[nodiscard]] MTPBusinessAwayMessageSchedule ToMTP(
		const AwaySchedule &data) {
	Expects(data.type != AwayScheduleType::Never);

	return (data.type == AwayScheduleType::Always)
		? MTP_businessAwayMessageScheduleAlways()
		: (data.type == AwayScheduleType::OutsideWorkingHours)
		? MTP_businessAwayMessageScheduleOutsideWorkHours()
		: MTP_businessAwayMessageScheduleCustom(
			MTP_int(data.customInterval.start),
			MTP_int(data.customInterval.end));
}

[[nodiscard]] MTPInputBusinessAwayMessage ToMTP(const AwaySettings &data) {
	using Flag = MTPDinputBusinessAwayMessage::Flag;
	return MTP_inputBusinessAwayMessage(
		MTP_flags(data.offlineOnly ? Flag::f_offline_only : Flag()),
		MTP_int(data.shortcutId),
		ToMTP(data.schedule),
		ForMessagesToMTP(data.recipients));
}

[[nodiscard]] MTPInputBusinessGreetingMessage ToMTP(
		const GreetingSettings &data) {
	return MTP_inputBusinessGreetingMessage(
		MTP_int(data.shortcutId),
		ForMessagesToMTP(data.recipients),
		MTP_int(data.noActivityDays));
}

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

[[nodiscard]] QJsonObject WorkingHoursToJson(const WorkingHours &data) {
	auto intervals = QJsonArray();
	for (const auto &interval : data.intervals.list) {
		auto pair = QJsonArray();
		pair.append(int(interval.start));
		pair.append(int(interval.end));
		intervals.append(std::move(pair));
	}
	auto object = QJsonObject();
	object.insert(u"tz"_q, data.timezoneId);
	object.insert(u"iv"_q, std::move(intervals));
	return object;
}

[[nodiscard]] WorkingHours WorkingHoursFromJson(const QJsonObject &object) {
	auto result = WorkingHours();
	result.timezoneId = object.value(u"tz"_q).toString();
	for (const auto &value : object.value(u"iv"_q).toArray()) {
		const auto pair = value.toArray();
		if (pair.size() != 2) {
			continue;
		}
		const auto interval = WorkingInterval{
			TimeId(pair[0].toVariant().toInt()),
			TimeId(pair[1].toVariant().toInt()),
		};
		if (interval) {
			result.intervals.list.push_back(interval);
		}
	}
	return result;
}

[[nodiscard]] QJsonObject BusinessLocationToJson(
		const BusinessLocation &data) {
	auto object = QJsonObject();
	object.insert(u"addr"_q, data.address);
	if (data.point) {
		object.insert(u"lat"_q, data.point->lat());
		object.insert(u"lon"_q, data.point->lon());
	}
	return object;
}

[[nodiscard]] BusinessLocation BusinessLocationFromJson(
		const QJsonObject &object) {
	auto result = BusinessLocation();
	result.address = object.value(u"addr"_q).toString();
	if (object.contains(u"lat"_q) && object.contains(u"lon"_q)) {
		result.point = LocationPoint(
			object.value(u"lat"_q).toVariant().toDouble(),
			object.value(u"lon"_q).toVariant().toDouble(),
			LocationPoint::NoAccessHash);
	}
	return result;
}

[[nodiscard]] QJsonObject ChatIntroToJson(const ChatIntro &data) {
	auto object = QJsonObject();
	object.insert(u"title"_q, data.title);
	object.insert(u"desc"_q, data.description);
	if (data.sticker) {
		object.insert(u"stk"_q, QJsonValue(qint64(data.sticker->id)));
	}
	return object;
}

[[nodiscard]] ChatIntro ChatIntroFromJson(
		not_null<Main::Session*> session,
		const QJsonObject &object) {
	auto result = ChatIntro();
	result.title = object.value(u"title"_q).toString();
	result.description = object.value(u"desc"_q).toString();
	const auto stickerId = object.value(u"stk"_q).toVariant().toULongLong();
	if (stickerId) {
		result.sticker = session->data().document(DocumentId(stickerId));
	}
	return result;
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
		users.append(QJsonValue(qint64(user->id.value)));
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

[[nodiscard]] QString ShortcutNameFor(
		not_null<Session*> owner,
		BusinessShortcutId id) {
	const auto &list = owner->shortcutMessages().shortcuts().list;
	const auto i = list.find(id);
	return (i != end(list)) ? i->second.name : QString();
}

[[nodiscard]] QJsonObject AwaySettingsToJson(
		not_null<Session*> owner,
		const AwaySettings &data) {
	auto object = QJsonObject();
	object.insert(u"rec"_q, BusinessRecipientsToJson(data.recipients));
	object.insert(u"t"_q, int(data.schedule.type));
	object.insert(u"cs"_q, int(data.schedule.customInterval.start));
	object.insert(u"ce"_q, int(data.schedule.customInterval.end));
	object.insert(u"sc"_q, QJsonValue(qint64(data.shortcutId)));
	object.insert(u"scn"_q, ShortcutNameFor(owner, data.shortcutId));
	object.insert(u"off"_q, data.offlineOnly);
	return object;
}

[[nodiscard]] AwaySettings AwaySettingsFromJson(
		not_null<Session*> owner,
		const QJsonObject &object) {
	auto result = AwaySettings();
	result.recipients = BusinessRecipientsFromJson(
		owner,
		object.value(u"rec"_q).toObject());
	const auto type = object.value(u"t"_q).toVariant().toInt();
	result.schedule.type = (type >= 0 && type <= 3)
		? AwayScheduleType(type)
		: AwayScheduleType::Never;
	result.schedule.customInterval = WorkingInterval{
		TimeId(object.value(u"cs"_q).toVariant().toInt()),
		TimeId(object.value(u"ce"_q).toVariant().toInt()),
	};
	result.shortcutId = BusinessShortcutId(
		object.value(u"sc"_q).toVariant().toLongLong());
	result.offlineOnly = object.value(u"off"_q).toBool();
	return result;
}

[[nodiscard]] QJsonObject GreetingSettingsToJson(
		not_null<Session*> owner,
		const GreetingSettings &data) {
	auto object = QJsonObject();
	object.insert(u"rec"_q, BusinessRecipientsToJson(data.recipients));
	object.insert(u"days"_q, data.noActivityDays);
	object.insert(u"sc"_q, QJsonValue(qint64(data.shortcutId)));
	object.insert(u"scn"_q, ShortcutNameFor(owner, data.shortcutId));
	return object;
}

[[nodiscard]] GreetingSettings GreetingSettingsFromJson(
		not_null<Session*> owner,
		const QJsonObject &object) {
	auto result = GreetingSettings();
	result.recipients = BusinessRecipientsFromJson(
		owner,
		object.value(u"rec"_q).toObject());
	result.noActivityDays = object.value(u"days"_q).toVariant().toInt();
	result.shortcutId = BusinessShortcutId(
		object.value(u"sc"_q).toVariant().toLongLong());
	return result;
}

} // namespace

BusinessInfo::BusinessInfo(not_null<Session*> owner)
: _owner(owner) {
}

BusinessInfo::~BusinessInfo() = default;

void BusinessInfo::applyLocalBusinessSettings() {
	const auto session = &_owner->session();
	if (!AyuSettings::localStars(session).localPremium()) {
		return;
	}
	const auto object = ReadLocalBusiness(session);
	if (object.isEmpty()) {
		return;
	}
	auto details = session->user()->businessDetails();
	if (object.contains(u"hours"_q)) {
		details.hours = WorkingHoursFromJson(
			object.value(u"hours"_q).toObject());
	}
	if (object.contains(u"location"_q)) {
		details.location = BusinessLocationFromJson(
			object.value(u"location"_q).toObject());
	}
	if (object.contains(u"intro"_q)) {
		details.intro = ChatIntroFromJson(
			session,
			object.value(u"intro"_q).toObject());
	}
	session->user()->setBusinessDetails(std::move(details));
	if (object.contains(u"away"_q)) {
		applyAwaySettings(AwaySettingsFromJson(
			_owner,
			object.value(u"away"_q).toObject()));
	}
	if (object.contains(u"greeting"_q)) {
		applyGreetingSettings(GreetingSettingsFromJson(
			_owner,
			object.value(u"greeting"_q).toObject()));
	}
}

void BusinessInfo::restoreLocalShortcuts() {
	const auto session = &_owner->session();
	if (!AyuSettings::localStars(session).localPremium()) {
		return;
	}
	auto &messages = _owner->shortcutMessages();
	if (!messages.shortcutsLoaded()) {
		return;
	}
	auto object = ReadLocalBusiness(session);
	auto changed = false;
	for (const auto &value : object.value(u"shortcuts"_q).toArray()) {
		const auto name = value.toString();
		if (name.isEmpty() || messages.lookupShortcutId(name)) {
			continue;
		}
		messages.emplaceShortcut(name);
		changed = true;
	}
	const auto remap = [&](const QString &key) {
		if (!object.contains(key)) {
			return;
		}
		auto settings = object.value(key).toObject();
		const auto name = settings.value(u"scn"_q).toString();
		if (name.isEmpty()) {
			return;
		}
		const auto id = messages.lookupShortcutId(name);
		if (id
			&& settings.value(u"sc"_q).toVariant().toLongLong() != id) {
			settings.insert(u"sc"_q, QJsonValue(qint64(id)));
			object.insert(key, settings);
			changed = true;
		}
	};
	remap(u"away"_q);
	remap(u"greeting"_q);
	if (changed) {
		WriteLocalBusiness(session, object);
		applyLocalBusinessSettings();
	}
}

void BusinessInfo::saveLocalShortcuts() {
	const auto session = &_owner->session();
	if (!AyuSettings::localStars(session).localPremium()) {
		return;
	}
	auto object = ReadLocalBusiness(session);
	auto names = QJsonArray();
	for (const auto &[id, shortcut]
		: _owner->shortcutMessages().shortcuts().list) {
		if (id < 0 && !shortcut.name.isEmpty()) {
			names.append(shortcut.name);
		}
	}
	object.insert(u"shortcuts"_q, std::move(names));
	WriteLocalBusiness(session, object);
}

void BusinessInfo::saveLocalShortcutMessage(
		const QString &name,
		const QString &text) {
	const auto session = &_owner->session();
	if (name.isEmpty()
		|| text.isEmpty()
		|| !AyuSettings::localStars(session).localPremium()) {
		return;
	}
	auto object = ReadLocalBusiness(session);
	auto all = object.value(u"scm"_q).toObject();
	auto list = all.value(name).toArray();
	list.append(text);
	while (list.size() > kMaxLocalShortcutMessages) {
		list.removeFirst();
	}
	all.insert(name, std::move(list));
	object.insert(u"scm"_q, std::move(all));
	WriteLocalBusiness(session, object);
}

std::vector<QString> BusinessInfo::localShortcutMessages(
		const QString &name) {
	const auto session = &_owner->session();
	const auto object = ReadLocalBusiness(session);
	const auto all = object.value(u"scm"_q).toObject();
	auto result = std::vector<QString>();
	for (const auto &value : all.value(name).toArray()) {
		const auto text = value.toString();
		if (!text.isEmpty()) {
			result.push_back(text);
		}
	}
	return result;
}

void BusinessInfo::removeLocalShortcutMessage(
		const QString &name,
		const QString &text) {
	const auto session = &_owner->session();
	if (name.isEmpty()
		|| text.isEmpty()
		|| !AyuSettings::localStars(session).localPremium()) {
		return;
	}
	auto object = ReadLocalBusiness(session);
	auto all = object.value(u"scm"_q).toObject();
	auto list = all.value(name).toArray();
	for (auto i = 0; i < list.size(); ++i) {
		if (list.at(i).toString() == text) {
			list.removeAt(i);
			break;
		}
	}
	if (list.isEmpty()) {
		all.remove(name);
	} else {
		all.insert(name, std::move(list));
	}
	object.insert(u"scm"_q, std::move(all));
	WriteLocalBusiness(session, object);
}

void BusinessInfo::renameLocalShortcutMessages(
		const QString &from,
		const QString &to) {
	const auto session = &_owner->session();
	if (from.isEmpty()
		|| to.isEmpty()
		|| from == to
		|| !AyuSettings::localStars(session).localPremium()) {
		return;
	}
	auto object = ReadLocalBusiness(session);
	auto all = object.value(u"scm"_q).toObject();
	if (!all.contains(from) || all.contains(to)) {
		return;
	}
	all.insert(to, all.take(from));
	object.insert(u"scm"_q, std::move(all));
	WriteLocalBusiness(session, object);
}

void BusinessInfo::clearLocalShortcutMessages(const QString &name) {
	const auto session = &_owner->session();
	if (name.isEmpty()
		|| !AyuSettings::localStars(session).localPremium()) {
		return;
	}
	auto object = ReadLocalBusiness(session);
	auto all = object.value(u"scm"_q).toObject();
	if (!all.contains(name)) {
		return;
	}
	all.remove(name);
	object.insert(u"scm"_q, std::move(all));
	WriteLocalBusiness(session, object);
}

void BusinessInfo::saveWorkingHours(
		WorkingHours data,
		Fn<void(QString)> fail) {
	const auto session = &_owner->session();
	auto details = session->user()->businessDetails();
	const auto &was = details.hours;
	if (was == data) {
		return;
	}
	if (AyuSettings::localStars(session).localPremium()) {
		auto object = ReadLocalBusiness(session);
		object.insert(u"hours"_q, WorkingHoursToJson(data));
		WriteLocalBusiness(session, object);
		details.hours = std::move(data);
		session->user()->setBusinessDetails(std::move(details));
		return;
	}

	using Flag = MTPaccount_UpdateBusinessWorkHours::Flag;
	session->api().request(MTPaccount_UpdateBusinessWorkHours(
		MTP_flags(data ? Flag::f_business_work_hours : Flag()),
		ToMTP(data)
	)).fail([=](const MTP::Error &error) {
		auto details = session->user()->businessDetails();
		details.hours = was;
		session->user()->setBusinessDetails(std::move(details));
		if (fail) {
			fail(error.type());
		}
	}).send();

	details.hours = std::move(data);
	session->user()->setBusinessDetails(std::move(details));
}

void BusinessInfo::saveChatIntro(ChatIntro data, Fn<void(QString)> fail) {
	const auto session = &_owner->session();
	auto details = session->user()->businessDetails();
	const auto &was = details.intro;
	if (was == data) {
		return;
	}
	if (AyuSettings::localStars(session).localPremium()) {
		auto object = ReadLocalBusiness(session);
		object.insert(u"intro"_q, ChatIntroToJson(data));
		WriteLocalBusiness(session, object);
		details.intro = std::move(data);
		session->user()->setBusinessDetails(std::move(details));
		return;
	} else {
		const auto session = &_owner->session();
		using Flag = MTPaccount_UpdateBusinessIntro::Flag;
		session->api().request(MTPaccount_UpdateBusinessIntro(
			MTP_flags(data ? Flag::f_intro : Flag()),
			MTP_inputBusinessIntro(
				MTP_flags(data.sticker
					? MTPDinputBusinessIntro::Flag::f_sticker
					: MTPDinputBusinessIntro::Flag()),
				MTP_string(data.title),
				MTP_string(data.description),
				(data.sticker
					? data.sticker->mtpInput()
					: MTP_inputDocumentEmpty()))
		)).fail([=](const MTP::Error &error) {
			auto details = session->user()->businessDetails();
			details.intro = was;
			session->user()->setBusinessDetails(std::move(details));
			if (fail) {
				fail(error.type());
			}
		}).send();
	}

	details.intro = std::move(data);
	session->user()->setBusinessDetails(std::move(details));
}

void BusinessInfo::saveLocation(
		BusinessLocation data,
		Fn<void(QString)> fail) {
	const auto session = &_owner->session();
	auto details = session->user()->businessDetails();
	const auto &was = details.location;
	if (was == data) {
		return;
	}
	if (AyuSettings::localStars(session).localPremium()) {
		auto object = ReadLocalBusiness(session);
		object.insert(u"location"_q, BusinessLocationToJson(data));
		WriteLocalBusiness(session, object);
		details.location = std::move(data);
		session->user()->setBusinessDetails(std::move(details));
		return;
	} else {
		const auto session = &_owner->session();
		using Flag = MTPaccount_UpdateBusinessLocation::Flag;
		session->api().request(MTPaccount_UpdateBusinessLocation(
			MTP_flags((data.point ? Flag::f_geo_point : Flag())
				| (data.address.isEmpty() ? Flag() : Flag::f_address)),
			(data.point
				? MTP_inputGeoPoint(
					MTP_flags(0),
					MTP_double(data.point->lat()),
					MTP_double(data.point->lon()),
					MTPint()) // accuracy_radius
				: MTP_inputGeoPointEmpty()),
			MTP_string(data.address)
		)).fail([=](const MTP::Error &error) {
			auto details = session->user()->businessDetails();
			details.location = was;
			session->user()->setBusinessDetails(std::move(details));
			if (fail) {
				fail(error.type());
			}
		}).send();
	}

	details.location = std::move(data);
	session->user()->setBusinessDetails(std::move(details));
}

void BusinessInfo::applyAwaySettings(AwaySettings data) {
	if (_awaySettings == data) {
		return;
	}
	_awaySettings = data;
	_awaySettingsChanged.fire({});
}

void BusinessInfo::saveAwaySettings(
		AwaySettings data,
		Fn<void(QString)> fail) {
	const auto &was = _awaySettings;
	if (was == data) {
		return;
	}
	if (data && !data.shortcutId) {
		if (fail) {
			fail(u"SHORTCUT_EMPTY"_q);
		}
		return;
	}
	const auto session = &_owner->session();
	if (AyuSettings::localStars(session).localPremium()) {
		auto object = ReadLocalBusiness(session);
		object.insert(u"away"_q, AwaySettingsToJson(_owner, data));
		WriteLocalBusiness(session, object);
		_awaySettings = std::move(data);
		_awaySettingsChanged.fire({});
		return;
	}
	if (!data || data.shortcutId) {
		using Flag = MTPaccount_UpdateBusinessAwayMessage::Flag;
		session->api().request(MTPaccount_UpdateBusinessAwayMessage(
			MTP_flags(data ? Flag::f_message : Flag()),
			data ? ToMTP(data) : MTPInputBusinessAwayMessage()
		)).fail([=](const MTP::Error &error) {
			_awaySettings = was;
			_awaySettingsChanged.fire({});
			if (fail) {
				fail(error.type());
			}
		}).send();
	}
	_awaySettings = std::move(data);
	_awaySettingsChanged.fire({});
}

bool BusinessInfo::awaySettingsLoaded() const {
	return _awaySettings.has_value();
}

AwaySettings BusinessInfo::awaySettings() const {
	return _awaySettings.value_or(AwaySettings());
}

rpl::producer<> BusinessInfo::awaySettingsChanged() const {
	return _awaySettingsChanged.events();
}

void BusinessInfo::applyGreetingSettings(GreetingSettings data) {
	if (_greetingSettings == data) {
		return;
	}
	_greetingSettings = data;
	_greetingSettingsChanged.fire({});
}

void BusinessInfo::saveGreetingSettings(
		GreetingSettings data,
		Fn<void(QString)> fail) {
	const auto &was = _greetingSettings;
	if (was == data) {
		return;
	}
	if (data && !data.shortcutId) {
		if (fail) {
			fail(u"SHORTCUT_EMPTY"_q);
		}
		return;
	}
	const auto session = &_owner->session();
	if (AyuSettings::localStars(session).localPremium()) {
		auto object = ReadLocalBusiness(session);
		object.insert(u"greeting"_q, GreetingSettingsToJson(_owner, data));
		WriteLocalBusiness(session, object);
		_greetingSettings = std::move(data);
		_greetingSettingsChanged.fire({});
		return;
	}
	if (!data || data.shortcutId) {
		using Flag = MTPaccount_UpdateBusinessGreetingMessage::Flag;
		session->api().request(
			MTPaccount_UpdateBusinessGreetingMessage(
				MTP_flags(data ? Flag::f_message : Flag()),
				data ? ToMTP(data) : MTPInputBusinessGreetingMessage())
		).fail([=](const MTP::Error &error) {
			_greetingSettings = was;
			_greetingSettingsChanged.fire({});
			if (fail) {
				fail(error.type());
			}
		}).send();
	}
	_greetingSettings = std::move(data);
	_greetingSettingsChanged.fire({});
}

bool BusinessInfo::greetingSettingsLoaded() const {
	return _greetingSettings.has_value();
}

GreetingSettings BusinessInfo::greetingSettings() const {
	return _greetingSettings.value_or(GreetingSettings());
}

rpl::producer<> BusinessInfo::greetingSettingsChanged() const {
	return _greetingSettingsChanged.events();
}

void BusinessInfo::preload() {
	preloadTimezones();
	applyLocalBusinessSettings();
}

void BusinessInfo::preloadTimezones() {
	if (!_timezones.current().list.empty() || _timezonesRequestId) {
		return;
	}
	_timezonesRequestId = _owner->session().api().request(
		MTPhelp_GetTimezonesList(MTP_int(_timezonesHash))
	).done([=](const MTPhelp_TimezonesList &result) {
		result.match([&](const MTPDhelp_timezonesList &data) {
			_timezonesHash = data.vhash().v;
			const auto proj = [](const MTPtimezone &result) {
				return Timezone{
					.id = qs(result.data().vid()),
					.name = qs(result.data().vname()),
					.utcOffset = result.data().vutc_offset().v,
				};
			};
			_timezones = Timezones{
				.list = ranges::views::all(
					data.vtimezones().v
				) | ranges::views::transform(
					proj
				) | ranges::to_vector,
			};
		}, [](const MTPDhelp_timezonesListNotModified &) {
		});
	}).send();
}

rpl::producer<Timezones> BusinessInfo::timezonesValue() const {
	const_cast<BusinessInfo*>(this)->preloadTimezones();
	return _timezones.value();
}

bool BusinessInfo::timezonesLoaded() const {
	return !_timezones.current().list.empty();
}

QString FindClosestTimezoneId(const std::vector<Timezone> &list) {
	const auto local = QDateTime::currentDateTime();
	const auto utc = QDateTime(local.date(), local.time(), Qt::UTC);
	const auto shift = base::unixtime::now() - (TimeId)::time(nullptr);
	const auto delta = int(utc.toSecsSinceEpoch())
		- int(local.toSecsSinceEpoch())
		- shift;
	const auto proj = [&](const Timezone &value) {
		auto distance = value.utcOffset - delta;
		while (distance > 12 * 3600) {
			distance -= 24 * 3600;
		}
		while (distance < -12 * 3600) {
			distance += 24 * 3600;
		}
		return std::abs(distance);
	};
	return ranges::min_element(list, ranges::less(), proj)->id;
}

} // namespace Data
