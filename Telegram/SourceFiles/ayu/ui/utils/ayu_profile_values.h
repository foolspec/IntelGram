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
enum class LocalProfileBadgeStyle;

namespace Data {
struct StarGift;
} // namespace Data

namespace Ui {
struct BotVerifyDetails;
} // namespace Ui

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
constexpr auto kMaxLocalStarsBalance = 1'000'000'000;
constexpr auto kMaxLocalStarsActivityLength = 6000;
constexpr auto kMaxLocalStarsCardNameLength = 64;
constexpr auto kMaxLocalStarsActivityCount = 200;
constexpr auto kMaxLocalGiftsCount = 200;
constexpr auto kMaxLocalGiftsLength = 65536;
constexpr auto kMaxLocalGiftMessageLength = 128;
constexpr auto kMaxLocalGiftAttributeLength = 128;
constexpr auto kMaxLocalStarsEarningsCount = 1000;
constexpr auto kMaxLocalStarsEarningsLength = 65536;
constexpr auto kLocalStarsProceedsMaturitySecs = 21 * 86400;
constexpr auto kMaxLocalPaidReactionsCount = 1000;
constexpr auto kMaxLocalPaidReactionsLength = 65536;
constexpr auto kMaxLocalBoostsLength = 4096;
constexpr auto kLocalBoostSlotsTotal = 4;
constexpr auto kMaxLocalGiveawaysCount = 200;
constexpr auto kMaxLocalGiveawaysLength = 65536;
constexpr auto kMaxLocalGiveawayPrizeLength = 128;
constexpr auto kMaxLocalBusinessLength = 65536;

enum class LocalStarsEarningSource {
	MiniApps = 0,
	Convert = 1,
	PaidReaction = 2,
};

struct LocalStarsEarning {
	int64 date = 0;
	int64 amount = 0;
	LocalStarsEarningSource source = LocalStarsEarningSource::MiniApps;
};

struct LocalStarsProceeds {
	int64 lifetime = 0;
	int64 total = 0;
	int64 available = 0;
	int64 nextMaturity = 0;
};

[[nodiscard]] std::vector<LocalStarsEarning> ParseLocalStarsEarnings(
	const QString &raw);
[[nodiscard]] QString SerializeLocalStarsEarnings(
	const std::vector<LocalStarsEarning> &earnings);
[[nodiscard]] QString NormalizeLocalStarsEarnings(QString earnings);
void AppendLocalStarsEarning(
	not_null<Main::Session*> session,
	LocalStarsEarning earning);
[[nodiscard]] LocalStarsProceeds ComputeLocalStarsProceeds(
	not_null<Main::Session*> session,
	int64 withdrawn,
	int64 now);
enum class LocalStarsActivityKind {
	TopUp = 0,
	GiftSent = 1,
	Earned = 2,
	Withdrawn = 3,
	Converted = 4,
};

struct LocalStarsActivityEntry {
	int64 date = 0;
	int64 amount = 0;
	LocalStarsActivityKind kind = LocalStarsActivityKind::TopUp;
	QString note;
};

[[nodiscard]] std::vector<LocalStarsActivityEntry> ParseLocalStarsActivity(
	const QString &raw);
[[nodiscard]] QString SerializeLocalStarsActivity(
	const std::vector<LocalStarsActivityEntry> &entries);
[[nodiscard]] QString NormalizeLocalStarsActivity(QString activity);
void AppendLocalStarsActivity(
	not_null<Main::Session*> session,
	LocalStarsActivityEntry entry);

[[nodiscard]] int64 LocalEarnClaimDay(
	not_null<Main::Session*> session,
	int index);
void SetLocalEarnClaimDay(
	not_null<Main::Session*> session,
	int index,
	int64 day);
[[nodiscard]] int NextLocalGiftSerial(
	not_null<Main::Session*> session,
	uint64 giftId);

[[nodiscard]] int64 LocalGiftConvertValue(int64 stars);
void RemoveLocalGift(
	not_null<Main::Session*> session,
	uint64 giftId,
	int64 date,
	const QString &message);
void RemoveLocalGift(
	not_null<Main::Session*> session,
	uint64 uid);

struct LocalGiftRecord {
	uint64 uid = 0;
	uint64 giftId = 0;
	uint64 stickerId = 0;
	uint64 recipientId = 0;
	QString title;
	int64 stars = 0;
	int64 date = 0;
	QString message;
	bool anonymous = false;
	bool upgraded = false;
	bool pinned = false;
	bool hidden = false;
	std::vector<int> collections;
	QString modelName;
	int modelRarity = 0;
	QString patternName;
	int patternRarity = 0;
	QString backdropName;
	int backdropId = 0;
	uint32 backdropCenter = 0;
	uint32 backdropEdge = 0;
	uint32 backdropPattern = 0;
	uint32 backdropText = 0;
	int number = 0;
};

[[nodiscard]] std::vector<LocalGiftRecord> ParseLocalGifts(
	const QString &raw);
[[nodiscard]] QString SerializeLocalGifts(
	const std::vector<LocalGiftRecord> &records);
[[nodiscard]] QString NormalizeLocalGifts(QString gifts);
void AppendLocalGift(
	not_null<Main::Session*> session,
	LocalGiftRecord record);
void UpdateLocalGift(
	not_null<Main::Session*> session,
	uint64 giftId,
	int64 date,
	const QString &message,
	Fn<void(LocalGiftRecord &record)> mutate);
void UpdateLocalGift(
	not_null<Main::Session*> session,
	uint64 uid,
	Fn<void(LocalGiftRecord &record)> mutate);

struct LocalPaidReaction {
	uint64 peerId = 0;
	int32 msgId = 0;
	int64 count = 0;
	int64 date = 0;
};

[[nodiscard]] std::vector<LocalPaidReaction> ParseLocalPaidReactions(
	const QString &raw);
[[nodiscard]] QString SerializeLocalPaidReactions(
	const std::vector<LocalPaidReaction> &reactions);
[[nodiscard]] QString NormalizeLocalPaidReactions(QString reactions);
void AppendLocalPaidReaction(
	not_null<Main::Session*> session,
	LocalPaidReaction reaction);

struct LocalBoostRecord {
	uint64 channelId = 0;
	int slot = 0;
};

[[nodiscard]] std::vector<LocalBoostRecord> ParseLocalBoosts(
	const QString &raw);
[[nodiscard]] QString SerializeLocalBoosts(
	const std::vector<LocalBoostRecord> &records);
[[nodiscard]] QString NormalizeLocalBoosts(QString boosts);
[[nodiscard]] std::vector<LocalBoostRecord> LocalBoosts(
	not_null<Main::Session*> session);
void SetLocalBoosts(
	not_null<Main::Session*> session,
	std::vector<LocalBoostRecord> records);
void AssignLocalBoostSlot(
	not_null<Main::Session*> session,
	uint64 channelId,
	int slot);
void ReleaseLocalBoostSlot(
	not_null<Main::Session*> session,
	int slot);
[[nodiscard]] int LocalBoostCountFor(
	not_null<Main::Session*> session,
	uint64 channelId);
[[nodiscard]] std::vector<int> LocalBoostSlotsFor(
	not_null<Main::Session*> session,
	uint64 channelId);

struct LocalBoostLevel {
	int level = 0;
	int boosts = 0;
	int thisLevelBoosts = 0;
	int nextLevelBoosts = 0;
	int mine = 0;
};

[[nodiscard]] LocalBoostLevel AdjustLocalBoostLevel(
	not_null<Main::Session*> session,
	uint64 channelId,
	LocalBoostLevel server);

struct LocalGiveawayRecord {
	int64 uid = 0;
	uint64 channelId = 0;
	int64 date = 0;
	int64 untilDate = 0;
	int quantity = 0;
	int months = 0;
	int64 credits = 0;
	QString prize;
	std::vector<QString> countries;
	std::vector<uint64> channels;
	std::vector<uint64> winners;
	bool onlyNewSubscribers = false;
	bool showWinners = false;
	bool resultsPosted = false;
};

[[nodiscard]] std::vector<LocalGiveawayRecord> ParseLocalGiveaways(
	const QString &raw);
[[nodiscard]] QString SerializeLocalGiveaways(
	const std::vector<LocalGiveawayRecord> &records);
[[nodiscard]] QString NormalizeLocalGiveaways(QString giveaways);
[[nodiscard]] std::vector<LocalGiveawayRecord> LocalGiveaways(
	not_null<Main::Session*> session);
[[nodiscard]] int64 AppendLocalGiveaway(
	not_null<Main::Session*> session,
	LocalGiveawayRecord record);
void UpdateLocalGiveaway(
	not_null<Main::Session*> session,
	const LocalGiveawayRecord &record);
[[nodiscard]] int LocalGiveawayBoosts(
	not_null<Main::Session*> session,
	uint64 channelId);

struct LocalPremiumGift {
	uint64 peerId = 0;
	int64 until = 0;
};

[[nodiscard]] std::vector<LocalPremiumGift> ParseLocalPremiumGifts(
	const QString &raw);
[[nodiscard]] QString SerializeLocalPremiumGifts(
	const std::vector<LocalPremiumGift> &gifts);
[[nodiscard]] QString NormalizeLocalPremiumGifts(QString gifts);
[[nodiscard]] bool HasLocalPremiumGift(
	not_null<Main::Session*> session,
	uint64 peerId);
void AddLocalPremiumGift(
	not_null<Main::Session*> session,
	uint64 peerId,
	int months);

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
[[nodiscard]] bool LocalProfileChannelShowcaseActive(
	not_null<UserData*> user);
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
[[nodiscard]] LocalProfileBadgeStyle LocalProfileBadgeStyleFor(
	not_null<PeerData*> peer);
[[nodiscard]] EmojiStatusId LocalProfileEmojiStatusId(
	not_null<PeerData*> peer);
[[nodiscard]] Ui::BotVerifyDetails LocalProfileBotVerifyDetails(
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
