// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <memory>
#include <vector>

class PeerData;
class UserData;
class ChannelData;
class QImage;

namespace Data {
struct StarGift;
} // namespace Data

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace Ayu {

constexpr auto kMaxLocalProfileNameLength = 128;
constexpr auto kMaxLocalProfileIdLength = 20;
constexpr auto kMaxLocalProfileUsernameLength = 64;
constexpr auto kMaxLocalProfileUsernamesCount = 20;
constexpr auto kMaxLocalProfileUsernamesLength = 512;
constexpr auto kMaxLocalProfilePhoneLength = 40;
constexpr auto kMaxLocalProfileAboutLength = 255;
constexpr auto kMaxLocalProfilePhotoLength = 4096;
constexpr auto kMaxLocalProfileGiftReferenceLength = 512;
constexpr auto kMaxLocalProfileGiftsCount = 7;
constexpr auto kMaxLocalProfileGiftsLength = 4096;

[[nodiscard]] QString NormalizeLocalProfileName(QString name);
[[nodiscard]] QString NormalizeLocalProfileId(QString id);
[[nodiscard]] QString NormalizeLocalProfileUsername(QString username);
[[nodiscard]] QString NormalizeLocalProfileUsernames(QString usernames);
[[nodiscard]] QString NormalizeLocalProfilePhone(QString phone);
[[nodiscard]] QString NormalizeLocalProfileAbout(QString about);
[[nodiscard]] QString NormalizeLocalProfilePhoto(QString path);
[[nodiscard]] QString NormalizeLocalProfileCloneId(QString id);
[[nodiscard]] QString NormalizeLocalProfileGift(QString gift);
[[nodiscard]] QString NormalizeLocalProfileGifts(QString gifts);
[[nodiscard]] bool LocalProfileNameActive(not_null<PeerData*> peer);
[[nodiscard]] bool LocalProfileIdActive(not_null<PeerData*> peer);
[[nodiscard]] bool LocalProfileUsernameActive(not_null<PeerData*> peer);
[[nodiscard]] bool LocalProfilePhoneActive(not_null<UserData*> user);
[[nodiscard]] bool LocalProfileAboutActive(not_null<PeerData*> peer);
[[nodiscard]] bool LocalProfilePhotoActive(not_null<PeerData*> peer);
[[nodiscard]] bool LocalProfileUserpicActive(not_null<PeerData*> peer);
[[nodiscard]] UserData *LocalProfileCloneUser(not_null<PeerData*> peer);
[[nodiscard]] ChannelData *LocalProfilePersonalChannel(
	not_null<UserData*> user);
[[nodiscard]] MsgId LocalProfilePersonalChannelMessageId(
	not_null<UserData*> user);
[[nodiscard]] QString LocalProfileName(not_null<PeerData*> peer);
[[nodiscard]] QString LocalProfileId(not_null<PeerData*> peer);
[[nodiscard]] QString LocalProfileUsername(not_null<PeerData*> peer);
[[nodiscard]] std::vector<QString> LocalProfileUsernames(
	not_null<PeerData*> peer);
[[nodiscard]] QString LocalProfilePhone(not_null<UserData*> user);
[[nodiscard]] QString LocalProfileAbout(not_null<PeerData*> peer);
[[nodiscard]] std::shared_ptr<QImage> LocalProfilePhotoImage(
	not_null<PeerData*> peer);
[[nodiscard]] EmojiStatusId LocalProfileEmojiStatusId(
	not_null<PeerData*> peer);
[[nodiscard]] std::vector<QString> LocalProfileGiftReferences(
	not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<QString> LocalProfileNameValue(
	not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<QString> LocalProfileIdValue(
	not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<QString> LocalProfileUsernameValue(
	not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<std::vector<QString>> LocalProfileUsernamesValue(
	not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<QString> LocalProfilePhoneValue(
	not_null<UserData*> user);
[[nodiscard]] rpl::producer<QString> LocalProfileAboutValue(
	not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<QString> LocalProfileCloneIdValue(
	not_null<PeerData*> peer);
[[nodiscard]] rpl::producer<ChannelData*> LocalProfilePersonalChannelValue(
	not_null<UserData*> user);
[[nodiscard]] auto LocalProfileGiftReferencesValue(
	not_null<PeerData*> peer) -> rpl::producer<std::vector<QString>>;
void ResolveLocalProfileGift(
	not_null<Main::Session*> session,
	QString reference,
	Fn<void(std::shared_ptr<Data::StarGift>)> done);
void ResolveLocalProfileGifts(
	not_null<Main::Session*> session,
	std::vector<QString> references,
	Fn<void(std::vector<std::shared_ptr<Data::StarGift>>)> done);
void ShowLocalProfileUsernameEditor(
	not_null<Window::SessionController*> controller,
	not_null<UserData*> user);
void RefreshLocalProfilePresentation(not_null<Main::Session*> session);

} // namespace Ayu

QString IDString(not_null<PeerData*> peer);
QString IDString(MsgId topicRootId);

rpl::producer<TextWithEntities> IDValue(not_null<PeerData*> peer);
rpl::producer<TextWithEntities> IDValue(MsgId topicRootId);
