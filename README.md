![IntelGram](branding/banner.png)

[![Download](https://img.shields.io/badge/Download-latest-2ea44f?style=flat&logo=github)](https://github.com/foolspec/IntelGram/releases/latest)
[![Telegram](https://img.shields.io/badge/Telegram-@intelgrams-229ED9?style=flat&logo=telegram)](https://t.me/intelgrams)

Imagine opening Telegram and your profile, your Stars balance, and the entire look of the app are exactly how you want them — without changing anything about your account. IntelGram is a Telegram Desktop build, based on AyuGram, focused on local profile customization, local Stars, and private on-device tools. Everything it adds runs on your device only.

## Installation

[![Release](https://img.shields.io/github/v/release/foolspec/IntelGram?display_name=tag&style=flat)](https://github.com/foolspec/IntelGram/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/foolspec/IntelGram/total?style=flat&color=fb9233)](https://github.com/foolspec/IntelGram/releases)

1. Download the latest build for your platform: [macOS Apple Silicon (DMG)](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-macOS-Apple-Silicon.dmg), [Windows x64 (ZIP)](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Windows-x64.zip), or [Linux x64 (tar.gz)](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Linux-x64.tar.gz).

2. macOS: open the DMG and drag `IntelGram.app` to Applications. Windows: extract the ZIP and run `IntelGram.exe`. Linux: extract the tarball and run `IntelGram`.

3. Open **IntelGram Preferences** and explore — or start with [Local Profile](#local-profile) and [Disguise Mode](#disguise-mode).

*macOS first launch: Control-click **Open** if Gatekeeper asks. The community build is ad-hoc signed, not Apple-notarized.*

## Local Stars

Your on-device Stars balance lives on Telegram's native **My Stars** row in Settings. **Top Up Balance** opens Telegram's own package list — the real 15 packages, from 100 to 150,000 Stars. Picking one opens Telegram's native checkout panel with the demo American Express card already saved; Pay adds the Stars to your local balance. Nothing is sent to Telegram or a payment provider.

## Local Profile

Set a local display name, UID, usernames, bio, profile photo, badges, emoji status, and verification in **IntelGram Preferences -> Local Profile & Collectibles**. You can clone the visible profile of any user already loaded in IntelGram by entering their UID.

### Disguise Mode

Triple-click the blue **IntelGram Desktop v...** heading in IntelGram Preferences and the app's visible identity switches to Telegram Desktop: app name, logo, Dock icon, tray icon, window title, menus, and About surface. Your local profile and collectibles stay active. Triple-click the **Telegram Desktop** drawer footer to come back.

### Collectibles

Browse Telegram's live collectible catalog, inspect numbered gifts in a scrollable grid, and paste Telegram, Getgems, or TON item links. Feature one collectible as the local profile backdrop and pin up to six around the local avatar.

### Local Channels

Enable **Local ownership** from any broadcast channel's **IntelGram** submenu to place persistent local text broadcasts directly in that channel's real timeline, with native reply, edit, pin, and reaction controls. Nothing is sent to Telegram — views and reactions update only this client.

## Appearance

### Liquid Glass

Turn on Liquid Glass in **IntelGram Preferences -> Glass & Motion** to layer adaptive tint, wallpaper-colored diffusion, cursor-reactive highlights, and refractive edges over native desktop blur. Chat list, message surfaces, and menus can be softened independently.

### Motion

Five global motion presets — Quick Snap, Smooth Flow, Diabolical, Springy, and the optional high-elasticity Liquid Bounce — apply across transitions, dialogs, drawers, and navigation.

### Themes and icons

Browse visual theme cards and cloud themes, import custom themes, and pick any local image as the chat background with a live preview. Choose the pink character icon, its profile-art variant, or any of twelve color variants from the in-app icon picker.

## Vault and Exports

Search messages, media, links, and filenames already received by IntelGram with an on-device full-text vault and a unified inbox across signed-in accounts. Jump-to-date, conversation statistics, saved moments, and locally observed edit and deletion history are built in.

Export selected messages, a chat, or an account to HTML, PDF, Markdown, JSON, or ZIP. Frozen Account Backup streams everything into one AES-256-GCM encrypted archive without leaving plaintext exports behind. Telegram's protected-content and self-destruct restrictions stay enforced throughout.

## Local means local

These controls only change how your own profile is rendered inside this IntelGram installation. They do not change your Telegram display name, photo, bio, username, UID, or collectible ownership, and other Telegram users do not see the local overrides.

## Building

IntelGram is built from the official AyuGram Desktop `v6.7.8` source, with the complete implementation in [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch). Every release includes SHA-256 checksums and platform validation notes. The [feature guide](FEATURE_GUIDE.md) walks through every workflow, and [FEATURES.md](FEATURES.md) documents the full feature surface.

## Credits

IntelGram is based on [AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop), which is based on [Telegram Desktop](https://github.com/telegramdesktop/tdesktop). Their upstream licenses and attribution remain in the source.

IntelGram custom features by **fool**.
