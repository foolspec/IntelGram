#!/usr/bin/env python3

import json
import shutil
from pathlib import Path


ROOT = Path.cwd()
BRANDING = Path(__file__).resolve().parent / "branding/icons"

COLOR_ICONS = (
	("alt", "Alt", "intelgram-square-01-midnight-purple"),
	("discord", "Discord", "intelgram-square-02-telegram-blue"),
	("spotify", "Spotify", "intelgram-square-03-deep-navy"),
	("extera", "Extera", "intelgram-square-04-cyan-teal"),
	("nothing", "Nothing", "intelgram-square-05-indigo"),
	("bard", "Bard", "intelgram-square-06-black-green"),
	("yaplus", "Yaplus", "intelgram-square-07-crimson"),
	("win95", "Win95", "intelgram-square-08-coral"),
	("extera2", "Extera2", "intelgram-square-09-slate-blue"),
	("lilac", "Lilac", "intelgram-square-10-lilac"),
	("whiteBlue", "WhiteBlue", "intelgram-square-11-white-blue"),
	("graphite", "Graphite", "intelgram-square-12-graphite"),
)


def replace(path: str, replacements: list[tuple[str, str]]) -> None:
	file = ROOT / path
	content = file.read_text(encoding="utf-8")
	for old, new in replacements:
		count = content.count(old)
		if count != 1:
			raise RuntimeError(f"{path}: expected one occurrence of {old!r}, found {count}")
		content = content.replace(old, new)
	file.write_text(content, encoding="utf-8")


def replace_or_verify(path: str, replacements: list[tuple[str, str]]) -> None:
	file = ROOT / path
	content = file.read_text(encoding="utf-8")
	for old, new in replacements:
		count = content.count(old)
		if count == 1:
			content = content.replace(old, new)
		elif count != 0 or content.count(new) != 1:
			raise RuntimeError(
				f"{path}: expected one occurrence of {old!r} or {new!r}, "
				f"found {count} and {content.count(new)}")
	file.write_text(content, encoding="utf-8")


def replace_all(path: str, old: str, new: str, expected: int) -> None:
	file = ROOT / path
	content = file.read_text(encoding="utf-8")
	count = content.count(old)
	if count != expected:
		raise RuntimeError(f"{path}: expected {expected} occurrences of {old!r}, found {count}")
	file.write_text(content.replace(old, new), encoding="utf-8")


def copy_asset(source: Path, target: str) -> None:
	destination = ROOT / target
	destination.parent.mkdir(parents=True, exist_ok=True)
	shutil.copy2(source, destination)


def write_mac_icon(name: str, source: Path) -> None:
	directory = ROOT / "Telegram/Telegram" / f"AppIcon-{name}.icon"
	assets = directory / "Assets"
	assets.mkdir(parents=True, exist_ok=True)
	shutil.copy2(source, assets / "app.png")
	(directory / "icon.json").write_text(json.dumps({
		"fill-specializations": [
			{"value": "system-light"},
			{
				"appearance": "dark",
				"value": {"solid": "extended-gray:0.75000,1.00000"},
			},
		],
		"groups": [{
			"layers": [{
				"blend-mode": "normal",
				"fill": "automatic",
				"glass": False,
				"image-name": "app.png",
				"name": "app",
				"opacity": 1,
				"position": {
					"scale": 1.0,
					"translation-in-points": [0, 0],
				},
			}],
			"shadow": {"kind": "neutral", "opacity": 0.5},
			"translucency": {"enabled": True, "value": 0.5},
		}],
		"supported-platforms": {"squares": ["macOS"]},
	}, indent=2) + "\n", encoding="utf-8")


replace("CMakeLists.txt", [
	('DESCRIPTION "AyuGram Desktop"', 'DESCRIPTION "IntelGram Desktop"'),
	('HOMEPAGE_URL "https://ayugram.one"', 'HOMEPAGE_URL "https://github.com/foolspec/IntelGram"'),
])

replace("Telegram/CMakeLists.txt", [
	('set(bundle_identifier "one.ayugram.AyuGramDesktop$<$<CONFIG:Debug>:Debug>")',
	 'set(bundle_identifier "io.github.foolspec.IntelGram$<$<CONFIG:Debug>:Debug>")'),
	('set(bundle_identifier "one.ayugram.AyuGramDesktop")',
	 'set(bundle_identifier "io.github.foolspec.IntelGram")'),
	('set(output_name "AyuGram")', 'set(output_name "IntelGram")'),
	('OR NOT "${output_name}" STREQUAL "AyuGram")',
	 'OR NOT "${output_name}" STREQUAL "IntelGram")'),
])

replace_all(
	"Telegram/CMakeLists.txt",
	"com.ayugram.desktop",
	"io.github.foolspec.intelgram",
	25,
)

replace("Telegram/SourceFiles/core/version.h", [
	('constexpr auto AppId = "{53F49750-6209-4FBF-9CA8-7A333C87D666}"_cs;',
	 'constexpr auto AppId = "{7AF76B28-0DD4-4CCB-BFA8-53CC3079650D}"_cs;'),
	('constexpr auto AppNameOld = "AyuGram for Windows"_cs;',
	 'constexpr auto AppNameOld = "IntelGram for Windows"_cs;'),
	('constexpr auto AppName = "AyuGram Desktop"_cs;',
	 'constexpr auto AppName = "IntelGram Desktop"_cs;'),
	('constexpr auto AppFile = "AyuGram"_cs;',
	 'constexpr auto AppFile = "IntelGram"_cs;'),
])

replace("Telegram/SourceFiles/window/main_window.cpp", [
	('setTitle((user.isEmpty() ? u"AyuGram"_q : user) + added);',
	 'setTitle((user.isEmpty() ? u"IntelGram"_q : user) + added);'),
])

replace("Telegram/SourceFiles/intro/intro_widget.cpp", [
	('QString("AyuGram Desktop v%1")', 'QString("IntelGram Desktop v%1")'),
])

replace("Telegram/SourceFiles/core/launcher.cpp", [
	('QApplication::setApplicationName(u"AyuGramDesktop"_q);',
	 'QApplication::setApplicationName(u"IntelGramDesktop"_q);'),
])

replace("Telegram/SourceFiles/core/application.cpp", [
	('u"https://github.com/AyuGram/AyuGramDesktop/releases"_q',
	 'u"https://github.com/foolspec/IntelGram/releases"_q'),
	('.shortAppName = u"AyuGram"_q,', '.shortAppName = u"IntelGram"_q,'),
])

replace("Telegram/SourceFiles/core/update_checker.cpp", [
	('return "https://t.me/AyuGramReleases";',
	 'return "https://github.com/foolspec/IntelGram/releases";'),
])

replace_all(
	"Telegram/SourceFiles/platform/linux/main_window_linux.cpp",
	'u"AyuGram"_q',
	'u"IntelGram"_q',
	2,
)

replace("Telegram/SourceFiles/platform/mac/window_title_mac.mm", [
	('p.drawText(titleRect, u"AyuGram"_q, style::al_center);',
	 'p.drawText(titleRect, u"IntelGram"_q, style::al_center);'),
])

replace("Telegram/SourceFiles/platform/mac/global_menu_mac.mm", [
	('u"About AyuGram"_q,', 'u"About IntelGram"_q,'),
	('_menuBar->addMenu(u"AyuGram"_q)', '_menuBar->addMenu(u"IntelGram"_q)'),
])

replace("Telegram/SourceFiles/core/crash_report_window.cpp", [
	('u"AyuGram"_q : title', 'u"IntelGram"_q : title'),
	('u"Could not start AyuGram Desktop!\\nYou can see complete log below:"_q',
	 'u"Could not start IntelGram Desktop!\\nYou can see complete log below:"_q'),
	('u"Last time AyuGram Desktop crashed :("_q',
	 'u"Last time IntelGram Desktop crashed :("_q'),
	('u"https://github.com/AyuGram/AyuGramDesktop"_q',
	 'u"https://github.com/foolspec/IntelGram"_q'),
	('u"AyuGram Crash Report"_q', 'u"IntelGram Crash Report"_q'),
])

replace_all(
	"Telegram/SourceFiles/core/crash_report_window.cpp",
	'u"Last time AyuGram Desktop was not closed properly."_q',
	'u"Last time IntelGram Desktop was not closed properly."_q',
	2,
)

replace("Telegram/SourceFiles/core/crash_reports.cpp", [
	('"AyuGram Desktop launch was not finished properly :( "',
	 '"IntelGram Desktop launch was not finished properly :( "'),
])

replace("Telegram/SourceFiles/boxes/about_box.cpp", [
	('"https://github.com/AyuGram/AyuGramDesktop")),',
	 '"https://github.com/foolspec/IntelGram")),'),
	('box->setTitle(rpl::single(u"AyuGram Desktop"_q));',
	 'box->setTitle(rpl::single(u"IntelGram Desktop"_q));'),
	('''box->addLeftButton(
		rpl::single(QString("@AyuGramReleases")),
		[box, controller]
		{
			box->closeBox();
			controller->showPeerByLink(Window::PeerByLinkInfo{
				.usernameOrId = QString("ayugramreleases"),
			});
		});''',
	 '''box->addLeftButton(
		rpl::single(QString("IntelGram on GitHub")),
		[box]
		{
			box->closeBox();
			File::OpenUrl("https://github.com/foolspec/IntelGram");
		});'''),
])

replace_all(
	"Telegram/SourceFiles/tray.cpp",
	'"Telegram", "AyuGram"',
	'"Telegram", "IntelGram"',
	2,
)

replace("Telegram/SourceFiles/export/output/export_output_html.cpp", [
	('"of AyuGram Desktop. Please update the application."',
	 '"of IntelGram Desktop. Please update the application."'),
])

replace("Telegram/SourceFiles/history/history_item_helpers.cpp", [
	('const auto siteLink = u"https://t.me/AyuGramReleases"_q;',
	 'const auto siteLink = u"https://github.com/foolspec/IntelGram/releases"_q;'),
	('.replace("Telegram", "AyuGram")', '.replace("Telegram", "IntelGram")'),
])

replace("Telegram/SourceFiles/window/window_main_menu.cpp", [
	('u"AyuGram Desktop"_q,', 'u"IntelGram Desktop"_q,'),
])

replace("Telegram/SourceFiles/window/notifications_manager_default.cpp", [
	('TextWithEntities{ u"AyuGram Desktop"_q }',
	 'TextWithEntities{ u"IntelGram Desktop"_q }'),
])

replace("Telegram/SourceFiles/ayu/ui/context_menu/context_menu.cpp", [
	('.text = u"AyuGram"_q,', '.text = u"IntelGram"_q,'),
])

replace("Telegram/SourceFiles/ayu/features/filters/filters_utils.cpp", [
	('titlePart.setBody("AyuGram Filters");',
	 'titlePart.setBody("IntelGram Filters");'),
])

replace("Telegram/SourceFiles/ayu/ui/components/message_preview.cpp", [
	('u"AyuGram Releases"_q);', 'u"IntelGram Releases"_q);'),
])

replace_all(
	"Telegram/SourceFiles/ayu/ui/components/avatar_corners_preview.cpp",
	'u"AyuGram Releases"_q',
	'u"IntelGram Releases"_q',
	2,
)

replace("Telegram/SourceFiles/ayu/ui/ayu_logo.cpp", [
	('+ u"tdata/AyuGram-"_q', '+ u"tdata/IntelGram-"_q'),
])

replace_all(
	"Telegram/SourceFiles/platform/win/windows_app_user_model_id.cpp",
	"AyuGram",
	"IntelGram",
	11,
)

replace_all(
	"Telegram/SourceFiles/platform/win/specific_win.cpp",
	"AyuGram.lnk",
	"IntelGram.lnk",
	2,
)

replace_all(
	"Telegram/SourceFiles/ayu/utils/windows_utils.cpp",
	"AyuGram",
	"IntelGram",
	8,
)

replace("Telegram/SourceFiles/_other/startup_task_win.cpp", [
	('L"\\\\AyuGram.exe"', 'L"\\\\IntelGram.exe"'),
])

replace_all(
	"Telegram/SourceFiles/_other/updater_win.cpp",
	"AyuGram.exe",
	"IntelGram.exe",
	4,
)

replace("Telegram/SourceFiles/_other/updater_osx.m", [
	('@"AyuGram.app"', '@"IntelGram.app"'),
	('@"/Contents/MacOS/AyuGram"', '@"/Contents/MacOS/IntelGram"'),
])

replace_all(
	"Telegram/Resources/winrc/Telegram.rc",
	"AyuGram Desktop",
	"IntelGram Desktop",
	2,
)

replace("Telegram/Resources/winrc/Updater.rc", [
	('"AyuGram Desktop Updater"', '"IntelGram Desktop Updater"'),
	('"AyuGram Desktop"', '"IntelGram Desktop"'),
])

replace("Telegram/build/setup.iss", [
	('#define MyAppShortName "AyuGram"', '#define MyAppShortName "IntelGram"'),
	('#define MyAppName "AyuGram Desktop"', '#define MyAppName "IntelGram Desktop"'),
	('#define MyAppURL "https://github.com/AyuGram"',
	 '#define MyAppURL "https://github.com/foolspec/IntelGram"'),
	('#define MyAppExeName "AyuGram.exe"', '#define MyAppExeName "IntelGram.exe"'),
])

replace("Telegram/SourceFiles/ayu/ui/settings/settings_ayu.cpp", [
	('.title = u"AyuGram"_q,', '.title = u"IntelGram"_q,'),
	('return rpl::single(QString("AyuGram"));',
	 'return rpl::single(QString("IntelGram"));'),
])

replace_or_verify("Telegram/SourceFiles/ayu/ui/settings/settings_main.cpp", [
	('QString("AyuGram Desktop v")', 'QString("IntelGram Desktop v")'),
])

replace("Telegram/SourceFiles/settings/sections/settings_notifications.cpp", [
	('u"AyuGram Desktop"_q, rectForName.width()',
	 'u"IntelGram Desktop"_q, rectForName.width()'),
])

replace("lib/xdg/com.ayugram.desktop.desktop", [
	('Name=AyuGram Desktop', 'Name=IntelGram Desktop'),
	('Comment=Desktop version of AyuGram - ToS breaking Telegram client',
	 'Comment=IntelGram client with local-only profile render overrides'),
	('TryExec=AyuGram', 'TryExec=IntelGram'),
	('Exec=env DESKTOPINTEGRATION=1 AyuGram -- %U',
	 'Exec=env DESKTOPINTEGRATION=1 IntelGram -- %U'),
	('Icon=com.ayugram.desktop', 'Icon=io.github.foolspec.intelgram'),
	('StartupWMClass=AyuGram', 'StartupWMClass=IntelGram'),
	('Exec=AyuGram -quit', 'Exec=IntelGram -quit'),
	('Name=Quit AyuGram', 'Name=Quit IntelGram'),
])

replace("lib/xdg/com.ayugram.desktop.service", [
	('Name=com.ayugram.desktop', 'Name=io.github.foolspec.intelgram'),
	('Exec=@CMAKE_INSTALL_FULL_BINDIR@/AyuGram',
	 'Exec=@CMAKE_INSTALL_FULL_BINDIR@/IntelGram'),
])

replace("lib/xdg/com.ayugram.desktop.metainfo.xml", [
	('<id>com.ayugram.desktop</id>', '<id>io.github.foolspec.intelgram</id>'),
	('<name>AyuGram Desktop</name>', '<name>IntelGram Desktop</name>'),
	('<summary>Desktop version of AyuGram - ToS breaking Telegram client</summary>',
	 '<summary>Telegram client with local-only profile render overrides</summary>'),
	('<p>Telegram Desktop with Ghost Mode, Anti-Recall and cool design.</p>',
	 '<p>IntelGram is based on AyuGram Desktop and adds client-render-only local profile controls.</p>'),
	('<url type="homepage">https://ayugram.one/</url>',
	 '<url type="homepage">https://github.com/foolspec/IntelGram</url>'),
	('<url type="faq">https://t.me/AyuGramFAQ</url>',
	 '<url type="faq">https://github.com/foolspec/IntelGram#using-intelgram</url>'),
	('<url type="bugtracker">https://t.me/ayugramchat/1262</url>',
	 '<url type="bugtracker">https://github.com/foolspec/IntelGram/issues</url>'),
	('<url type="vcs-browser">https://github.com/AyuGram/AyuGramDesktop</url>',
	 '<url type="vcs-browser">https://github.com/foolspec/IntelGram</url>'),
	('<url type="contribute">https://github.com/AyuGram/AyuGramDesktop/blob/dev/.github/CONTRIBUTING.md</url>',
	 '<url type="contribute">https://github.com/foolspec/IntelGram</url>'),
	('<keyword>ayugram</keyword>', '<keyword>intelgram</keyword>'),
	('<launchable type="desktop-id">com.ayugram.desktop.desktop</launchable>',
	 '<launchable type="desktop-id">io.github.foolspec.intelgram.desktop</launchable>'),
	('<binary>AyuGram</binary>', '<binary>IntelGram</binary>'),
])

for old, new in (
	("com.ayugram.desktop.desktop", "io.github.foolspec.intelgram.desktop"),
	("com.ayugram.desktop.service", "io.github.foolspec.intelgram.service"),
	("com.ayugram.desktop.metainfo.xml", "io.github.foolspec.intelgram.metainfo.xml"),
):
	source = ROOT / "lib/xdg" / old
	(ROOT / "lib/xdg" / new).write_bytes(source.read_bytes())

replace("Telegram/SourceFiles/ayu/ui/ayu_logo.h", [
	('ICON(EXTERA2, "extera2");', '''ICON(EXTERA2, "extera2");
ICON(LILAC, "lilac");
ICON(WHITEBLUE, "whiteBlue");
ICON(GRAPHITE, "graphite");'''),
])

replace("Telegram/SourceFiles/ayu/ui/components/icon_picker.cpp", [
	('''const QVector<QString> icons{
	AyuAssets::DEFAULT_ICON,
	AyuAssets::ALT_ICON,
	AyuAssets::DISCORD_ICON,
	AyuAssets::SPOTIFY_ICON,
	AyuAssets::EXTERA_ICON,
	AyuAssets::NOTHING_ICON,
	AyuAssets::BARD_ICON,
	AyuAssets::YAPLUS_ICON,
	AyuAssets::WIN95_ICON,
	AyuAssets::CHIBI_ICON,
	AyuAssets::CHIBI2_ICON,
	AyuAssets::EXTERA2_ICON,
};''', '''const QVector<QString> icons{
	AyuAssets::DEFAULT_ICON,
	AyuAssets::CHIBI_ICON,
	AyuAssets::ALT_ICON,
	AyuAssets::DISCORD_ICON,
	AyuAssets::SPOTIFY_ICON,
	AyuAssets::EXTERA_ICON,
	AyuAssets::NOTHING_ICON,
	AyuAssets::BARD_ICON,
	AyuAssets::YAPLUS_ICON,
	AyuAssets::WIN95_ICON,
	AyuAssets::EXTERA2_ICON,
	AyuAssets::LILAC_ICON,
	AyuAssets::WHITEBLUE_ICON,
	AyuAssets::GRAPHITE_ICON,
};'''),
])

replace("Telegram/SourceFiles/ayu/ayu_settings.cpp", [
	('s._appIcon = j.value("appIcon", defaults._appIcon.current());', '''const auto appIcon = j.value("appIcon", defaults._appIcon.current());
		s._appIcon = (appIcon == AyuAssets::CHIBI2_ICON)
			? AyuAssets::DEFAULT_ICON
			: appIcon;'''),
])

replace("Telegram/CMakeLists.txt", [
	('''            Chibi2
            Extera2
        )''', '''            Chibi2
            Extera2
            Lilac
            WhiteBlue
            Graphite
        )'''),
	('XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_ALTERNATE_APPICON_NAMES AppIcon-Alt AppIcon-Discord AppIcon-Spotify AppIcon-Extera AppIcon-Nothing AppIcon-Bard AppIcon-Yaplus AppIcon-Win95 AppIcon-Chibi AppIcon-Chibi2 AppIcon-Extera2',
	 'XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_ALTERNATE_APPICON_NAMES "AppIcon-Alt AppIcon-Discord AppIcon-Spotify AppIcon-Extera AppIcon-Nothing AppIcon-Bard AppIcon-Yaplus AppIcon-Win95 AppIcon-Chibi AppIcon-Chibi2 AppIcon-Extera2 AppIcon-Lilac AppIcon-WhiteBlue AppIcon-Graphite"'),
])

qrc = "Telegram/Resources/qrc/ayu/ayu.qrc"
replace(qrc, [
	('<file alias="art/ayu/default/app.svg">../../art/ayu/default/app.svg</file>',
	 '<file alias="art/ayu/default/app.png">../../art/ayu/default/app.png</file>'),
	('''        <file alias="art/ayu/extera2/app_icon.ico">../../art/ayu/extera2/app_icon.ico</file>''', '''        <file alias="art/ayu/extera2/app_icon.ico">../../art/ayu/extera2/app_icon.ico</file>
        <file alias="art/ayu/lilac/app.png">../../art/ayu/lilac/app.png</file>
        <file alias="art/ayu/lilac/app_icon.ico">../../art/ayu/lilac/app_icon.ico</file>
        <file alias="art/ayu/whiteBlue/app.png">../../art/ayu/whiteBlue/app.png</file>
        <file alias="art/ayu/whiteBlue/app_icon.ico">../../art/ayu/whiteBlue/app_icon.ico</file>
        <file alias="art/ayu/graphite/app.png">../../art/ayu/graphite/app.png</file>
        <file alias="art/ayu/graphite/app_icon.ico">../../art/ayu/graphite/app_icon.ico</file>'''),
])

legacy_extensions = {
	"alt": "svg",
	"discord": "svg",
	"spotify": "svg",
	"extera": "svg",
	"nothing": "svg",
	"bard": "svg",
	"yaplus": "svg",
	"win95": "png",
	"extera2": "svg",
}
for name, _, _ in COLOR_ICONS:
	if name not in legacy_extensions:
		continue
	extension = legacy_extensions[name]
	old = f'<file alias="art/ayu/{name}/app.{extension}">../../art/ayu/{name}/app.{extension}</file>'
	new = f'<file alias="art/ayu/{name}/app.png">../../art/ayu/{name}/app.png</file>'
	if old != new:
		replace(qrc, [(old, new)])

front = BRANDING / "character/masters/intelgram-pink-front.png"
profile = BRANDING / "character/masters/intelgram-pink-profile.png"
front_macos = BRANDING / "character/macOS/intelgram-pink-front.png"
profile_macos = BRANDING / "character/macOS/intelgram-pink-profile.png"
front_ico = BRANDING / "character/Windows/intelgram-pink-front.ico"
profile_ico = BRANDING / "character/Windows/intelgram-pink-profile.ico"

for name, image, ico in (
	("default", front, front_ico),
	("chibi", profile, profile_ico),
	("chibi2", front, front_ico),
):
	copy_asset(image, f"Telegram/Resources/art/ayu/{name}/app.png")
	copy_asset(ico, f"Telegram/Resources/art/ayu/{name}/app_icon.ico")

write_mac_icon("Default", front_macos)
write_mac_icon("Chibi", profile_macos)
write_mac_icon("Chibi2", front_macos)

for name, mac_name, filename in COLOR_ICONS:
	image = BRANDING / "color/masters" / f"{filename}.png"
	mac_image = BRANDING / "color/macOS" / f"{filename}.png"
	ico = BRANDING / "color/Windows" / f"{filename}.ico"
	copy_asset(image, f"Telegram/Resources/art/ayu/{name}/app.png")
	copy_asset(ico, f"Telegram/Resources/art/ayu/{name}/app_icon.ico")
	write_mac_icon(mac_name, mac_image)

primary_sizes = {
	"icon16.png": 16,
	"icon16@2x.png": 32,
	"icon32.png": 32,
	"icon32@2x.png": 64,
	"icon48.png": 48,
	"icon48@2x.png": 96,
	"icon64.png": 64,
	"icon64@2x.png": 128,
	"icon128.png": 128,
	"icon128@2x.png": 256,
	"icon256.png": 256,
	"icon256@2x.png": 512,
	"icon512.png": 512,
	"icon512@2x.png": 1024,
	"icon_round512@2x.png": 1024,
	"logo_256.png": 256,
	"logo_256_no_margin.png": 256,
}
for filename, size in primary_sizes.items():
	source = BRANDING / "character/Linux/hicolor" / f"{size}x{size}/apps/intelgram-pink-front.png"
	copy_asset(source, f"Telegram/Resources/art/{filename}")

copy_asset(front_ico, "Telegram/Resources/art/icon256.ico")
