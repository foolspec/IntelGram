#pragma once

class ChannelData;
class QString;

namespace Window {
class SessionController;
} // namespace Window

namespace Ayu {

[[nodiscard]] QString LocalChannelDisplayTitle(ChannelData *channel);
void ShowLocalChannelWorkspace(
	not_null<Window::SessionController*> controller);

} // namespace Ayu
