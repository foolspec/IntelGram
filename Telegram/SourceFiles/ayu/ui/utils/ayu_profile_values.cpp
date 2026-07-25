// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/utils/ayu_profile_values.h"

#include "api/api_premium.h"
#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "ayu/utils/telegram_helpers.h"
#include "base/weak_ptr.h"
#include "boxes/abstract_box.h"
#include "boxes/peers/edit_peer_common.h"
#include "boxes/username_box.h"
#include "data/data_channel.h"
#include "data/data_changes.h"
#include "data/data_emoji_statuses.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_star_gift.h"
#include "data/data_user.h"
#include "lang/lang_text_entity.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "rpl/combine.h"
#include "ui/image/image.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/special_fields.h"
#include "ui/widgets/labels.h"
#include "ui/vertical_list.h"
#include "window/window_session_controller.h"

#include "styles/style_boxes.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

constexpr auto kMaxChannelId = -1000000000000;

namespace Ayu {
namespace {

using UpdateFlag = Data::PeerUpdate::Flag;
using GiftResult = std::shared_ptr<Data::StarGift>;
using GiftResults = std::vector<GiftResult>;

struct GiftResolveState {
	GiftResults results;
	Fn<void(GiftResults)> done;
	int pending = 0;
};

struct LocalProfilePhotoCache {
	QString path;
	qint64 modified = 0;
	std::shared_ptr<QImage> image;
};

[[nodiscard]] UserData *ResolveLocalProfileCloneUser(
		not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return nullptr;
	}
	const auto id = NormalizeLocalProfileCloneId(
		AyuSettings::getInstance().localProfileCloneId());
	if (id.isEmpty()) {
		return nullptr;
	}
	const auto value = id.toULongLong();
	const auto result = value
		? peer->owner().userLoaded(UserId(value))
		: nullptr;
	return (result && result != peer.get()) ? result : nullptr;
}

[[nodiscard]] bool IsTonNftAddress(const QString &value) {
	static const auto Friendly = QRegularExpression(
		u"^(?:EQ|UQ|kQ|0Q)[A-Za-z0-9_-]{46}$"_q);
	static const auto Raw = QRegularExpression(
		u"^-?[0-9]+:[A-Fa-f0-9]{64}$"_q);
	return Friendly.match(value).hasMatch() || Raw.match(value).hasMatch();
}

[[nodiscard]] QString NormalizeGiftSlug(QString value) {
	auto result = QString();
	for (const auto ch : value) {
		if (ch.isLetterOrNumber() || ch == '-' || ch == '_') {
			result.append(ch);
		}
		if (result.size() >= kMaxLocalProfileGiftReferenceLength) {
			break;
		}
	}
	return result;
}

void AddLocalProfileGiftReference(
		std::vector<QString> &result,
		QString reference) {
	reference = NormalizeLocalProfileGift(std::move(reference));
	if (reference.isEmpty()) {
		return;
	}
	const auto duplicate = ranges::any_of(result, [&](const auto &entry) {
		return entry.compare(reference, Qt::CaseInsensitive) == 0;
	});
	if (!duplicate && result.size() < kMaxLocalProfileGiftsCount) {
		result.push_back(std::move(reference));
	}
}

[[nodiscard]] std::vector<QString> ParseLocalProfileGifts(
		QString featured,
		QString gifts) {
	auto result = std::vector<QString>();
	AddLocalProfileGiftReference(result, std::move(featured));

	auto token = QString();
	const auto flush = [&] {
		AddLocalProfileGiftReference(result, std::move(token));
		token = QString();
	};
	for (const auto ch : gifts) {
		if (ch.isSpace() || ch == ',' || ch == ';') {
			flush();
		} else {
			token.append(ch);
		}
	}
	flush();
	return result;
}

[[nodiscard]] QString GiftSlugFromMetadata(const QByteArray &bytes) {
	const auto document = QJsonDocument::fromJson(bytes);
	if (!document.isObject()) {
		return {};
	}
	const auto root = document.object();
	auto name = root.value(u"metadata"_q).toObject().value(u"name"_q).toString();
	if (name.isEmpty()) {
		name = root.value(u"nft_content"_q).toObject().value(u"name"_q).toString();
	}
	if (name.isEmpty()) {
		name = root.value(u"name"_q).toString();
	}
	static const auto NameAndNumber = QRegularExpression(
		u"^(.+?)\\s*#([0-9]+)$"_q);
	const auto match = NameAndNumber.match(name.trimmed());
	if (!match.hasMatch()) {
		return NormalizeGiftSlug(name);
	}
	auto title = match.captured(1);
	title.remove(QRegularExpression(u"[^A-Za-z0-9]"_q));
	return title.isEmpty()
		? QString()
		: title + '-' + match.captured(2);
}

[[nodiscard]] QNetworkAccessManager *LocalProfileNetworkManager() {
	static const auto Result = new QNetworkAccessManager(
		QCoreApplication::instance());
	return Result;
}

void ResolveGiftSlug(
		not_null<Main::Session*> session,
		const QString &slug,
		Fn<void(GiftResult)> done) {
	const auto callback = std::make_shared<Fn<void(GiftResult)>>(std::move(done));
	session->api().request(MTPpayments_GetUniqueStarGift(
		MTP_string(slug)
	)).done([=](const MTPpayments_UniqueStarGift &result) {
		const auto &data = result.data();
		session->data().processUsers(data.vusers());
		auto gift = Api::FromTL(session, data.vgift());
		(*callback)(gift
			? std::make_shared<Data::StarGift>(std::move(*gift))
			: nullptr);
	}).fail([=](const MTP::Error &) {
		(*callback)(nullptr);
	}).send();
}

void ResolveGiftAddress(
		not_null<Main::Session*> session,
		const QString &address,
		Fn<void(GiftResult)> done) {
	const auto callback = std::make_shared<Fn<void(GiftResult)>>(std::move(done));
	const auto encoded = QString::fromLatin1(QUrl::toPercentEncoding(address));
	const auto reply = LocalProfileNetworkManager()->get(QNetworkRequest(
		QUrl(u"https://tonapi.io/v2/nfts/"_q + encoded)));
	const auto weak = base::make_weak(session);
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		const auto bytes = reply->readAll();
		const auto status = reply->attribute(
			QNetworkRequest::HttpStatusCodeAttribute).toInt();
		const auto failed = reply->error() != QNetworkReply::NoError
			|| status < 200
			|| status >= 300;
		reply->deleteLater();
		if (failed || !weak) {
			(*callback)(nullptr);
			return;
		}
		const auto slug = GiftSlugFromMetadata(bytes);
		if (slug.isEmpty()) {
			(*callback)(nullptr);
			return;
		}
		ResolveGiftSlug(weak.get(), slug, [=](GiftResult gift) {
			(*callback)(std::move(gift));
		});
	});
}

[[nodiscard]] QString ResolveLocalProfileName(
		not_null<PeerData*> peer,
		QString realName,
		bool enabled,
		QString localName) {
	if (!peer->isSelf()) {
		return realName;
	}
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		return clone->name();
	}
	if (!enabled) {
		return realName;
	}
	localName = NormalizeLocalProfileName(std::move(localName));
	return localName.isEmpty() ? realName : localName;
}

void AddLocalProfileUsername(
		std::vector<QString> &result,
		QString username) {
	username = NormalizeLocalProfileUsername(std::move(username));
	if (username.isEmpty()) {
		return;
	}
	const auto duplicate = ranges::any_of(result, [&](const auto &entry) {
		return entry.compare(username, Qt::CaseInsensitive) == 0;
	});
	if (!duplicate && result.size() < kMaxLocalProfileUsernamesCount) {
		result.push_back(std::move(username));
	}
}

[[nodiscard]] std::vector<QString> ParseLocalProfileUsernames(
		QString primary,
		QString usernames) {
	auto result = std::vector<QString>();
	AddLocalProfileUsername(result, std::move(primary));

	auto token = QString();
	const auto flush = [&] {
		AddLocalProfileUsername(result, std::move(token));
		token = QString();
	};
	for (const auto ch : usernames) {
		if (ch.isSpace() || ch == ',' || ch == ';') {
			flush();
		} else {
			token.append(ch);
		}
	}
	flush();
	return result;
}

[[nodiscard]] std::vector<QString> RealProfileUsernames(
		not_null<PeerData*> peer) {
	if (const auto user = peer->asUser()) {
		return user->usernames();
	} else if (const auto channel = peer->asChannel()) {
		return channel->usernames();
	}
	return {};
}

[[nodiscard]] QString ResolveLocalProfileUsername(
		not_null<PeerData*> peer,
		QString realUsername,
		bool enabled,
		QString localUsername,
		QString localUsernames) {
	if (!peer->isSelf()) {
		return realUsername;
	}
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		return clone->username();
	}
	if (!enabled) {
		return realUsername;
	}
	const auto local = ParseLocalProfileUsernames(
		std::move(localUsername),
		std::move(localUsernames));
	return local.empty() ? realUsername : local.front();
}

[[nodiscard]] std::vector<QString> ResolveLocalProfileUsernames(
		not_null<PeerData*> peer,
		std::vector<QString> realUsernames,
		bool enabled,
		QString localUsername,
		QString localUsernames) {
	if (!peer->isSelf()) {
		return realUsernames;
	}
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		return clone->usernames();
	}
	if (!enabled) {
		return realUsernames;
	}
	auto local = ParseLocalProfileUsernames(
		std::move(localUsername),
		std::move(localUsernames));
	return local.empty() ? realUsernames : std::move(local);
}

[[nodiscard]] QString ResolveLocalProfilePhone(
		not_null<UserData*> user,
		QString realPhone,
		bool enabled,
		QString localPhone) {
	if (!user->isSelf()) {
		return realPhone;
	}
	if (const auto clone = ResolveLocalProfileCloneUser(user)) {
		return clone->phone();
	}
	if (!enabled) {
		return realPhone;
	}
	localPhone = NormalizeLocalProfilePhone(std::move(localPhone));
	return localPhone.isEmpty() ? realPhone : localPhone;
}

[[nodiscard]] QString ResolveLocalProfileAbout(
		not_null<PeerData*> peer,
		QString realAbout,
		bool enabled,
		QString localAbout) {
	if (!peer->isSelf()) {
		return realAbout;
	}
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		return clone->about();
	}
	if (!enabled) {
		return realAbout;
	}
	localAbout = NormalizeLocalProfileAbout(std::move(localAbout));
	return localAbout.isEmpty() ? realAbout : localAbout;
}

class LocalUsernameEditor final : public Ui::RpWidget {
public:
	LocalUsernameEditor(
		not_null<Ui::RpWidget*> parent,
		QString value);

	[[nodiscard]] Ui::UsernameInput *field() const;
	[[nodiscard]] rpl::producer<> changes() const;
	[[nodiscard]] rpl::producer<> submits() const;

protected:
	void resizeEvent(QResizeEvent *e) override;

private:
	const style::margins &_padding;
	object_ptr<Ui::UsernameInput> _field;
	rpl::event_stream<> _changes;
	rpl::event_stream<> _submits;
};

LocalUsernameEditor::LocalUsernameEditor(
	not_null<Ui::RpWidget*>,
	QString value)
: _padding(st::usernamePadding)
, _field(
	this,
	st::defaultInputField,
	rpl::single(u"@username"_q),
	std::move(value),
	QString()) {
	resize(width(), _padding.top() + _field->height());
	connect(_field, &Ui::MaskedInputField::changed, [=] {
		_changes.fire({});
	});
	connect(_field, &Ui::MaskedInputField::submitted, [=] {
		_submits.fire({});
	});
}

Ui::UsernameInput *LocalUsernameEditor::field() const {
	return _field.data();
}

rpl::producer<> LocalUsernameEditor::changes() const {
	return _changes.events();
}

rpl::producer<> LocalUsernameEditor::submits() const {
	return _submits.events();
}

void LocalUsernameEditor::resizeEvent(QResizeEvent *e) {
	_field->resize(
		width() - _padding.left() - _padding.right(),
		_field->height());
	_field->moveToLeft(_padding.left(), _padding.top());
}

} // namespace

QString NormalizeLocalProfileName(QString name) {
	auto result = name.trimmed();
	if (result.size() > kMaxLocalProfileNameLength) {
		result.truncate(kMaxLocalProfileNameLength);
	}
	return result;
}

QString NormalizeLocalProfileId(QString id) {
	auto result = QString();
	for (const auto ch : id.trimmed()) {
		if (ch >= '0' && ch <= '9') {
			result.append(ch);
		}
		if (result.size() >= kMaxLocalProfileIdLength) {
			break;
		}
	}
	return result;
}

QString NormalizeLocalProfileUsername(QString username) {
	auto result = username.trimmed();
	const auto prefixes = {
		u"https://t.me/"_q,
		u"http://t.me/"_q,
		u"t.me/"_q,
	};
	for (const auto &prefix : prefixes) {
		if (result.startsWith(prefix, Qt::CaseInsensitive)) {
			result = result.mid(prefix.size());
			break;
		}
	}
	while (result.startsWith('@')) {
		result = result.mid(1);
	}
	auto filtered = QString();
	for (const auto ch : result) {
		if (ch.isLetterOrNumber()
			|| ch == '_'
			|| ch == '-'
			|| ch == '.') {
			filtered.append(ch);
		}
		if (filtered.size() >= kMaxLocalProfileUsernameLength) {
			break;
		}
	}
	return filtered;
}

QString NormalizeLocalProfileUsernames(QString usernames) {
	auto result = QStringList();
	for (auto &username : ParseLocalProfileUsernames(
			QString(),
			std::move(usernames))) {
		result.push_back(std::move(username));
	}
	auto joined = result.join(u", "_q);
	if (joined.size() > kMaxLocalProfileUsernamesLength) {
		joined.truncate(kMaxLocalProfileUsernamesLength);
	}
	return joined;
}

QString NormalizeLocalProfilePhone(QString phone) {
	auto result = QString();
	phone = phone.trimmed();
	for (const auto ch : phone) {
		if (ch.isDigit() || (result.isEmpty() && ch == '+')) {
			result.append(ch);
		}
		if (result.size() >= kMaxLocalProfilePhoneLength) {
			break;
		}
	}
	return result;
}

QString NormalizeLocalProfileAbout(QString about) {
	auto result = about.trimmed();
	if (result.size() > kMaxLocalProfileAboutLength) {
		result.truncate(kMaxLocalProfileAboutLength);
	}
	return result;
}

QString NormalizeLocalProfilePhoto(QString path) {
	auto result = path.trimmed();
	if (result.size() > kMaxLocalProfilePhotoLength) {
		result.truncate(kMaxLocalProfilePhotoLength);
	}
	return result;
}

QString NormalizeLocalProfileCloneId(QString id) {
	return NormalizeLocalProfileId(std::move(id));
}

QString NormalizeLocalProfileGift(QString gift) {
	auto result = gift.trimmed();
	if (result.isEmpty()) {
		return {};
	}
	const auto url = QUrl::fromUserInput(result);
	if (url.isValid() && !url.host().isEmpty()) {
		const auto segments = url.path().split('/', Qt::SkipEmptyParts);
		const auto host = url.host().toLower();
		if ((host == u"t.me"_q
				|| host == u"telegram.me"_q
				|| host == u"telegram.dog"_q)
			&& segments.size() >= 2
			&& segments[segments.size() - 2].compare(
				u"nft"_q,
				Qt::CaseInsensitive) == 0) {
			return NormalizeGiftSlug(segments.back());
		}
		for (auto i = segments.size(); i != 0; --i) {
			const auto candidate = QUrl::fromPercentEncoding(
				segments[i - 1].toUtf8());
			if (IsTonNftAddress(candidate)) {
				return candidate.left(kMaxLocalProfileGiftReferenceLength);
			}
		}
	}
	if (url.scheme().compare(u"tg"_q, Qt::CaseInsensitive) == 0) {
		const auto slug = QUrlQuery(url).queryItemValue(u"slug"_q);
		if (!slug.isEmpty()) {
			return NormalizeGiftSlug(slug);
		}
	}
	if (IsTonNftAddress(result)) {
		return result.left(kMaxLocalProfileGiftReferenceLength);
	}
	return NormalizeGiftSlug(std::move(result));
}

QString NormalizeLocalProfileGifts(QString gifts) {
	auto result = QStringList();
	for (auto &gift : ParseLocalProfileGifts(QString(), std::move(gifts))) {
		result.push_back(std::move(gift));
	}
	auto joined = result.join(u", "_q);
	if (joined.size() > kMaxLocalProfileGiftsLength) {
		joined.truncate(kMaxLocalProfileGiftsLength);
	}
	return joined;
}

bool LocalProfileNameActive(not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return false;
	}
	if (ResolveLocalProfileCloneUser(peer)) {
		return true;
	}
	const auto &settings = AyuSettings::getInstance();
	return settings.localProfileNameEnabled()
		&& !NormalizeLocalProfileName(settings.localProfileName()).isEmpty();
}

bool LocalProfileIdActive(not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return false;
	}
	if (ResolveLocalProfileCloneUser(peer)) {
		return true;
	}
	const auto &settings = AyuSettings::getInstance();
	return settings.localProfileIdEnabled()
		&& !NormalizeLocalProfileId(settings.localProfileId()).isEmpty();
}

bool LocalProfileUsernameActive(not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return false;
	}
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		return !clone->usernames().empty() || !clone->username().isEmpty();
	}
	const auto &settings = AyuSettings::getInstance();
	return settings.localProfileUsernameEnabled()
		&& !ParseLocalProfileUsernames(
			settings.localProfileUsername(),
			settings.localProfileUsernames()).empty();
}

bool LocalProfilePhoneActive(not_null<UserData*> user) {
	if (!user->isSelf()) {
		return false;
	}
	if (const auto clone = ResolveLocalProfileCloneUser(user)) {
		return !clone->phone().isEmpty();
	}
	const auto &settings = AyuSettings::getInstance();
	return settings.localProfilePhoneEnabled()
		&& !NormalizeLocalProfilePhone(settings.localProfilePhone()).isEmpty();
}

bool LocalProfileAboutActive(not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return false;
	}
	if (ResolveLocalProfileCloneUser(peer)) {
		return true;
	}
	const auto &settings = AyuSettings::getInstance();
	return settings.localProfileAboutEnabled()
		&& !NormalizeLocalProfileAbout(settings.localProfileAbout()).isEmpty();
}

UserData *LocalProfileCloneUser(not_null<PeerData*> peer) {
	return ResolveLocalProfileCloneUser(peer);
}

ChannelData *LocalProfilePersonalChannel(not_null<UserData*> user) {
	const auto clone = ResolveLocalProfileCloneUser(user);
	const auto source = clone ? clone : user.get();
	const auto channelId = source->personalChannelId();
	return channelId ? source->owner().channel(channelId).get() : nullptr;
}

MsgId LocalProfilePersonalChannelMessageId(not_null<UserData*> user) {
	const auto clone = ResolveLocalProfileCloneUser(user);
	const auto source = clone ? clone : user.get();
	return source->personalChannelMessageId();
}

std::shared_ptr<QImage> LocalProfilePhotoImage(
		not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return nullptr;
	}
	const auto path = NormalizeLocalProfilePhoto(
		AyuSettings::getInstance().localProfilePhoto());
	const auto info = QFileInfo(path);
	if (path.isEmpty() || !info.isFile()) {
		return nullptr;
	}
	static auto cache = LocalProfilePhotoCache();
	const auto modified = info.lastModified().toMSecsSinceEpoch();
	if (cache.path == path && cache.modified == modified) {
		return cache.image;
	}
	cache.path = path;
	cache.modified = modified;
	cache.image = nullptr;
	auto image = Images::Read({ .path = path }).image;
	if (image.isNull()) {
		return nullptr;
	}
	const auto side = std::min(image.width(), image.height());
	image = image.copy(
		(image.width() - side) / 2,
		(image.height() - side) / 2,
		side,
		side);
	constexpr auto kMaxLocalProfilePhotoSide = 1024;
	if (side > kMaxLocalProfilePhotoSide) {
		image = image.scaled(
			kMaxLocalProfilePhotoSide,
			kMaxLocalProfilePhotoSide,
			Qt::IgnoreAspectRatio,
			Qt::SmoothTransformation);
	}
	cache.image = std::make_shared<QImage>(std::move(image));
	return cache.image;
}

bool LocalProfilePhotoActive(not_null<PeerData*> peer) {
	return LocalProfilePhotoImage(peer) != nullptr;
}

bool LocalProfileUserpicActive(not_null<PeerData*> peer) {
	if (LocalProfilePhotoActive(peer)) {
		return true;
	}
	const auto clone = ResolveLocalProfileCloneUser(peer);
	return clone && clone->hasUserpic();
}

QString LocalProfileName(not_null<PeerData*> peer) {
	const auto &settings = AyuSettings::getInstance();
	return ResolveLocalProfileName(
		peer,
		peer->name(),
		settings.localProfileNameEnabled(),
		settings.localProfileName());
}

QString LocalProfileId(not_null<PeerData*> peer) {
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		return QString::number(getBareID(clone));
	}
	if (!LocalProfileIdActive(peer)) {
		return QString::number(getBareID(peer));
	}
	return NormalizeLocalProfileId(
		AyuSettings::getInstance().localProfileId());
}

QString LocalProfileUsername(not_null<PeerData*> peer) {
	const auto &settings = AyuSettings::getInstance();
	return ResolveLocalProfileUsername(
		peer,
		peer->username(),
		settings.localProfileUsernameEnabled(),
		settings.localProfileUsername(),
		settings.localProfileUsernames());
}

std::vector<QString> LocalProfileUsernames(not_null<PeerData*> peer) {
	const auto &settings = AyuSettings::getInstance();
	return ResolveLocalProfileUsernames(
		peer,
		RealProfileUsernames(peer),
		settings.localProfileUsernameEnabled(),
		settings.localProfileUsername(),
		settings.localProfileUsernames());
}

QString LocalProfilePhone(not_null<UserData*> user) {
	const auto &settings = AyuSettings::getInstance();
	return ResolveLocalProfilePhone(
		user,
		user->phone(),
		settings.localProfilePhoneEnabled(),
		settings.localProfilePhone());
}

QString LocalProfileAbout(not_null<PeerData*> peer) {
	const auto &settings = AyuSettings::getInstance();
	return ResolveLocalProfileAbout(
		peer,
		peer->about(),
		settings.localProfileAboutEnabled(),
		settings.localProfileAbout());
}

EmojiStatusId LocalProfileEmojiStatusId(not_null<PeerData*> peer) {
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		return clone->emojiStatusId();
	}
	return peer->emojiStatusId();
}

std::vector<QString> LocalProfileGiftReferences(not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return {};
	}
	if (const auto clone = ResolveLocalProfileCloneUser(peer)) {
		const auto status = clone->emojiStatusId();
		return (status.collectible && !status.collectible->slug.isEmpty())
			? std::vector<QString>{ status.collectible->slug }
			: std::vector<QString>();
	}
	const auto &settings = AyuSettings::getInstance();
	return settings.localProfileGiftEnabled()
		? ParseLocalProfileGifts(
			settings.localProfileGift(),
			settings.localProfileGifts())
		: std::vector<QString>();
}

rpl::producer<QString> LocalProfileNameValue(not_null<PeerData*> peer) {
	auto name = peer->session().changes().peerFlagsValue(
		peer,
		UpdateFlag::Name
	) | rpl::map([=] {
		return peer->name();
	});
	if (!peer->isSelf()) {
		return name;
	}
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		std::move(name),
		settings.localProfileNameEnabledValue(),
		settings.localProfileNameValue(),
		settings.localProfileCloneIdValue()
	) | rpl::map([=](
			QString realName,
			bool enabled,
			QString localName,
			const QString &) {
		return ResolveLocalProfileName(
			peer,
			std::move(realName),
			enabled,
			std::move(localName));
	});
}

rpl::producer<QString> LocalProfileIdValue(not_null<PeerData*> peer) {
	if (!peer->isSelf()) {
		return rpl::single(LocalProfileId(peer));
	}
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		settings.localProfileIdEnabledValue(),
		settings.localProfileIdValue(),
		settings.localProfileCloneIdValue()
	) | rpl::map([=](bool, const QString &, const QString &) {
		return LocalProfileId(peer);
	});
}

rpl::producer<QString> LocalProfileUsernameValue(not_null<PeerData*> peer) {
	auto username = rpl::merge(
		peer->session().changes().peerFlagsValue(peer, UpdateFlag::Username),
		peer->session().changes().peerFlagsValue(peer, UpdateFlag::Usernames)
	) | rpl::map([=] {
		return peer->username();
	});
	if (!peer->isSelf()) {
		return username;
	}
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		std::move(username),
		settings.localProfileUsernameEnabledValue(),
		settings.localProfileUsernameValue(),
		settings.localProfileUsernamesValue(),
		settings.localProfileCloneIdValue()
	) | rpl::map([=](
			QString realUsername,
			bool enabled,
			QString localUsername,
			QString localUsernames,
			const QString &) {
		return ResolveLocalProfileUsername(
			peer,
			std::move(realUsername),
			enabled,
			std::move(localUsername),
			std::move(localUsernames));
	});
}

rpl::producer<std::vector<QString>> LocalProfileUsernamesValue(
		not_null<PeerData*> peer) {
	auto usernames = rpl::merge(
		peer->session().changes().peerFlagsValue(peer, UpdateFlag::Username),
		peer->session().changes().peerFlagsValue(peer, UpdateFlag::Usernames)
	) | rpl::map([=] {
		return RealProfileUsernames(peer);
	});
	if (!peer->isSelf()) {
		return usernames;
	}
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		std::move(usernames),
		settings.localProfileUsernameEnabledValue(),
		settings.localProfileUsernameValue(),
		settings.localProfileUsernamesValue(),
		settings.localProfileCloneIdValue()
	) | rpl::map([=](
			std::vector<QString> realUsernames,
			bool enabled,
			QString localUsername,
			QString localUsernames,
			const QString &) {
		return ResolveLocalProfileUsernames(
			peer,
			std::move(realUsernames),
			enabled,
			std::move(localUsername),
			std::move(localUsernames));
	});
}

rpl::producer<QString> LocalProfilePhoneValue(not_null<UserData*> user) {
	auto phone = user->session().changes().peerFlagsValue(
		user,
		UpdateFlag::PhoneNumber
	) | rpl::map([=] {
		return user->phone();
	});
	if (!user->isSelf()) {
		return phone;
	}
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		std::move(phone),
		settings.localProfilePhoneEnabledValue(),
		settings.localProfilePhoneValue(),
		settings.localProfileCloneIdValue()
	) | rpl::map([=](
			QString realPhone,
			bool enabled,
			QString localPhone,
			const QString &) {
		return ResolveLocalProfilePhone(
			user,
			std::move(realPhone),
			enabled,
			std::move(localPhone));
	});
}

rpl::producer<QString> LocalProfileAboutValue(not_null<PeerData*> peer) {
	auto about = peer->session().changes().peerFlagsValue(
		peer,
		UpdateFlag::About
	) | rpl::map([=] {
		return peer->about();
	});
	if (!peer->isSelf()) {
		return about;
	}
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		std::move(about),
		settings.localProfileAboutEnabledValue(),
		settings.localProfileAboutValue(),
		settings.localProfileCloneIdValue()
	) | rpl::map([=](
			QString realAbout,
			bool enabled,
			QString localAbout,
			const QString &) {
		return ResolveLocalProfileAbout(
			peer,
			std::move(realAbout),
			enabled,
			std::move(localAbout));
	});
}

rpl::producer<QString> LocalProfileCloneIdValue(
		not_null<PeerData*> peer) {
	return peer->isSelf()
		? AyuSettings::getInstance().localProfileCloneIdValue()
		: rpl::single(QString());
}

rpl::producer<ChannelData*> LocalProfilePersonalChannelValue(
		not_null<UserData*> user) {
	const auto sourceValue = [=]() -> rpl::producer<ChannelData*> {
		const auto clone = ResolveLocalProfileCloneUser(user);
		const auto source = clone ? clone : user.get();
		return source->session().changes().peerFlagsValue(
			source,
			UpdateFlag::PersonalChannel
		) | rpl::map([=] {
			const auto channelId = source->personalChannelId();
			return channelId
				? source->owner().channel(channelId).get()
				: nullptr;
		});
	};
	return LocalProfileCloneIdValue(user) | rpl::map([=](const QString &) {
		return sourceValue();
	}) | rpl::flatten_latest();
}

auto LocalProfileGiftReferencesValue(not_null<PeerData*> peer)
-> rpl::producer<std::vector<QString>> {
	if (!peer->isSelf()) {
		return rpl::single(std::vector<QString>());
	}
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		settings.localProfileGiftEnabledValue(),
		settings.localProfileGiftValue(),
		settings.localProfileGiftsValue(),
		settings.localProfileCloneIdValue()
	) | rpl::map([=](bool, const QString &, const QString &, const QString &) {
		return LocalProfileGiftReferences(peer);
	});
}

void ResolveLocalProfileGift(
		not_null<Main::Session*> session,
		QString reference,
		Fn<void(GiftResult)> done) {
	reference = NormalizeLocalProfileGift(std::move(reference));
	if (reference.isEmpty()) {
		done(nullptr);
	} else if (IsTonNftAddress(reference)) {
		ResolveGiftAddress(session, reference, std::move(done));
	} else {
		ResolveGiftSlug(session, reference, std::move(done));
	}
}

void ResolveLocalProfileGifts(
		not_null<Main::Session*> session,
		std::vector<QString> references,
		Fn<void(GiftResults)> done) {
	if (references.empty()) {
		done({});
		return;
	}
	const auto state = std::make_shared<GiftResolveState>();
	state->results.resize(references.size());
	state->done = std::move(done);
	state->pending = int(references.size());
	for (auto i = size_t(0); i != references.size(); ++i) {
		ResolveLocalProfileGift(
			session,
			std::move(references[i]),
			[=](GiftResult gift) {
				state->results[i] = std::move(gift);
				if (!--state->pending) {
					state->done(std::move(state->results));
				}
			});
	}
}

void ShowLocalProfileUsernameEditor(
		not_null<Window::SessionController*> controller,
		not_null<UserData*>) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(tr::lng_username_title());
		const auto container = box->verticalLayout();
		const auto editor = box->addRow(
			object_ptr<LocalUsernameEditor>(
				box,
				AyuSettings::getInstance().localProfileUsername()),
			style::margins());
		const auto field = editor->field();
		field->setMaxLength(kMaxLocalProfileUsernameLength);
		box->setFocusCallback([=] {
			field->setFocusFast();
		});

		auto checkInfo = rpl::single(rpl::empty) | rpl::then(
			editor->changes()
		) | rpl::map([=] {
			auto name = field->getLastText().trimmed();
			if (name.startsWith('@')) {
				name.remove(0, 1);
			}
			if (name.isEmpty()) {
				return UsernameCheckInfo{
					.type = UsernameCheckInfo::Type::Default,
				};
			}
			for (const auto ch : name) {
				if ((ch < 'A' || ch > 'Z')
					&& (ch < 'a' || ch > 'z')
					&& (ch < '0' || ch > '9')
					&& ch != '_') {
					return UsernameCheckInfo{
						.type = UsernameCheckInfo::Type::Error,
						.text = { tr::lng_username_bad_symbols(tr::now) },
					};
				}
			}
			if (name.size() < Ui::EditPeer::kMinUsernameLength) {
				return UsernameCheckInfo{
					.type = UsernameCheckInfo::Type::Error,
					.text = { tr::lng_username_too_short(tr::now) },
				};
			}
			return UsernameCheckInfo{
				.type = UsernameCheckInfo::Type::Good,
				.text = { tr::lng_username_available(tr::now) },
			};
		});
		AddUsernameCheckLabel(container, std::move(checkInfo));
		auto description = rpl::combine(
			tr::lng_username_description1(tr::rich),
			tr::lng_username_description2(tr::rich)
		) | rpl::map([](TextWithEntities first, TextWithEntities second) {
			return first.append("\n\n").append(std::move(second));
		});
		container->add(object_ptr<Ui::DividerLabel>(
			container,
			object_ptr<Ui::FlatLabel>(
				container,
				std::move(description),
				st::boxDividerLabel),
			st::defaultBoxDividerLabelPadding));

		const auto save = [=] {
			auto &settings = AyuSettings::getInstance();
			settings.setLocalProfileUsername(field->getLastText());
			settings.setLocalProfileUsernameEnabled(true);
			box->closeBox();
		};
		editor->submits(
		) | rpl::on_next(save, editor->lifetime());
		box->addButton(tr::lng_settings_save(), save);
		box->addButton(tr::lng_cancel(), [=] {
			box->closeBox();
		});
	}));
}

void RefreshLocalProfilePresentation(not_null<Main::Session*> session) {
	session->changes().peerUpdated(
		session->user(),
		UpdateFlag::Name
			| UpdateFlag::Username
			| UpdateFlag::Usernames
			| UpdateFlag::Photo
			| UpdateFlag::About
			| UpdateFlag::PhoneNumber
			| UpdateFlag::EmojiStatus
			| UpdateFlag::ColorProfile
			| UpdateFlag::VerifyInfo
			| UpdateFlag::PersonalChannel);
}

} // namespace Ayu

QString IDString(const not_null<PeerData*> peer) {
	auto resultId = Ayu::LocalProfileId(peer);

	const auto &settings = AyuSettings::getInstance();
	if (settings.showPeerId() == PeerIdDisplay::BotApi) {
		if (peer->isChannel()) {
			resultId = QString::number(peerToChannel(peer->id).bare - kMaxChannelId).prepend("-");
		} else if (peer->isChat()) {
			resultId = resultId.prepend("-");
		}
	}

	return resultId;
}

QString IDString(MsgId topicRootId) {
	return QString::number(topicRootId.bare);
}

rpl::producer<TextWithEntities> IDValue(not_null<PeerData*> peer) {
	auto &settings = AyuSettings::getInstance();
	return rpl::combine(
		settings.showPeerIdValue(),
		settings.localProfileIdEnabledValue(),
		settings.localProfileIdValue()
	) | rpl::map([=](PeerIdDisplay display, bool, const QString &) {
		return (display == PeerIdDisplay::Hidden)
			? TextWithEntities()
			: tr::marked(IDString(peer));
	});
}

rpl::producer<TextWithEntities> IDValue(MsgId topicRootId) {
	return AyuSettings::getInstance().showPeerIdValue(
	) | rpl::map([=](PeerIdDisplay display) {
		return (display == PeerIdDisplay::Hidden)
			? TextWithEntities()
			: tr::marked(IDString(topicRootId));
	});
}
