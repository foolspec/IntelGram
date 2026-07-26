#pragma once

class ChannelData;
class History;
class HistoryItem;
class PeerData;
struct TextWithTags;

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
	bool scheduled);
[[nodiscard]] bool InterceptLocalChannelUnsupportedSend(
	not_null<Window::SessionController*> controller,
	PeerData *peer);
void ShowLocalChannelOwnership(
	not_null<Window::SessionController*> controller,
	not_null<ChannelData*> channel);
[[nodiscard]] bool IsLocalChannelBroadcast(HistoryItem *item);
void DeleteLocalChannelBroadcast(not_null<HistoryItem*> item);

} // namespace Ayu
