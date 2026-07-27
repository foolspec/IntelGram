#include "ayu/ui/discreet_mode.h"

#include "ayu/ayu_settings.h"
#include "ayu/ui/ayu_logo.h"
#include "core/application.h"
#include "tray.h"
#include "window/main_window.h"

#ifdef Q_OS_MAC
#include "platform/mac/global_menu_mac.h"
#endif

#ifdef Q_OS_WIN
#include "ayu/utils/windows_utils.h"
#endif

#include <QtGui/QGuiApplication>

namespace Ayu {
namespace {

void ApplyDisplayIdentity() {
	QGuiApplication::setApplicationDisplayName(VisibleClientName());
}

void RefreshApplicationIcon() {
#ifdef Q_OS_WIN
	AyuAssets::loadAppIco();
	reloadAppIconFromTaskBar();
#endif
	Window::OverrideApplicationIcon(AyuAssets::currentAppLogo());
	Core::App().refreshApplicationIcon();
	Core::App().tray().updateIconCounters();
}

} // namespace

bool DiscreetModeEnabled() {
	return AyuSettings::getInstance().discreetMode();
}

rpl::producer<bool> DiscreetModeValue() {
	return AyuSettings::getInstance().discreetModeValue();
}

QString VisibleClientName() {
	return DiscreetModeEnabled()
		? u"Telegram Desktop"_q
		: u"IntelGram Desktop"_q;
}

QString VisibleClientShortName() {
	return DiscreetModeEnabled() ? u"Telegram"_q : u"IntelGram"_q;
}

QString VisibleClientUrl() {
	return DiscreetModeEnabled()
		? u"https://desktop.telegram.org"_q
		: u"https://github.com/foolspec/IntelGram"_q;
}

void InitializeDiscreetMode() {
	ApplyDisplayIdentity();
}

void SetDiscreetMode(bool enabled) {
	auto &settings = AyuSettings::getInstance();
	if (settings.discreetMode() == enabled) {
		return;
	}
	settings.setDiscreetMode(enabled);
	ApplyDisplayIdentity();
	RefreshApplicationIcon();
#ifdef Q_OS_MAC
	Platform::DestroyGlobalMenu();
	Platform::CreateGlobalMenu();
#endif
}

} // namespace Ayu
