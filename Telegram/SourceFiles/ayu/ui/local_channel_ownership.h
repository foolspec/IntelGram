#pragma once

class ChannelData;
class History;
class HistoryItem;
class PeerData;
class QString;
struct TextWithTags;

namespace Data {
struct ReactionId;
} // namespace Data

namespace Window {
class SessionController;
} // namespace Window

namespace Ayu {

enum class LocalChannelBroadcastResult {
	NotHandled,
	Saved,
	Blocked,
};

[[nodiscard]] bool LocalChannelOwnershipActive(PeerData *peer);
void EnsureLocalChannelBroadcasts(not_null<History*> history);
[[nodiscard]] LocalChannelBroadcastResult HandleLocalChannelBroadcast(
	not_null<History*> history,
	const TextWithTags &text,
	bool scheduled,
	HistoryItem *replyTo = nullptr);
[[nodiscard]] bool InterceptLocalChannelUnsupportedSend(
	not_null<Window::SessionController*> controller,
	PeerData *peer);
void ShowLocalChannelOwnership(
	not_null<Window::SessionController*> controller,
	not_null<ChannelData*> channel);
[[nodiscard]] bool IsLocalChannelBroadcast(HistoryItem *item);
void RefreshLocalChannelBroadcastMetrics(HistoryItem *item);
[[nodiscard]] bool LocalChannelBroadcastPinned(HistoryItem *item);
void ToggleLocalChannelBroadcastPinned(not_null<HistoryItem*> item);
void ShowEditLocalChannelBroadcast(
	not_null<Window::SessionController*> controller,
	not_null<HistoryItem*> item);
void ConfirmDeleteLocalChannelBroadcast(
	not_null<Window::SessionController*> controller,
	not_null<HistoryItem*> item);
[[nodiscard]] QString LocalChannelBroadcastLink(HistoryItem *item);
[[nodiscard]] bool IncrementLocalChannelBroadcastReaction(
	not_null<HistoryItem*> item,
	const Data::ReactionId &reaction);
void DeleteLocalChannelBroadcast(not_null<HistoryItem*> item);

} // namespace Ayu
