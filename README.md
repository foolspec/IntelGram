<div align="center">
<a href="#installation">
<img src="branding/banner.png"></a>
</div>

<div align="center">
<img src="branding/badges/Trending.svg" alt="GitHub Trending" height="55">
<img src="branding/badges/TrendShift.svg" alt="TrendShift" height="55">
</div>

<div align="center">
<a href="https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-macOS-Apple-Silicon.dmg">
<img src="branding/badges/Download-macOS.svg" alt="Download for macOS" height="32"></a>
<a href="https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Windows-x64.zip">
<img src="branding/badges/Download-Windows.svg" alt="Download for Windows" height="32"></a>
<a href="https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Linux-x64.tar.gz">
<img src="branding/badges/Download-Linux.svg" alt="Download for Linux" height="32"></a>
</div>

<div align="center">
<a href="https://github.com/foolspec/IntelGram/releases/latest">
<img src="branding/badges/Download.svg" alt="Download"></a>
</div>

<div align="center">
<a href="https://github.com/foolspec/IntelGram/discussions/new/choose">
<img src="https://img.shields.io/github/discussions-search?query=repo%3Afoolspec%2FIntelGram%20is%3Aopen&style=flat&logo=TheConversation&logoColor=white&label=Ask%20Question&color=6e40c9" alt="Ask Question"></a>
<a href="https://github.com/foolspec/IntelGram/issues/new?template=bug-report.yaml">
<img src="https://img.shields.io/github/issues-search?query=repo%3Afoolspec%2FIntelGram%20is%3Aopen%20label%3Abug&style=flat&logo=ghostery&logoColor=white&label=New%20Bug&color=fdc12c" alt="New Bug"></a>
<a href="https://t.me/intelgrams">
<img src="https://img.shields.io/badge/Telegram-@intelgrams-229ED9?style=flat&logo=telegram" alt="Telegram"></a>
<a href="https://github.com/foolspec/IntelGram/blob/main/LICENSE">
<img src="https://img.shields.io/github/license/foolspec/IntelGram?style=flat&color=blueviolet" alt="License"></a>
<a href="https://github.com/foolspec/IntelGram/commits/main">
<img src="https://img.shields.io/github/last-commit/foolspec/IntelGram?style=flat&color=ff69b4" alt="Last commit"></a>
<a href="https://isocpp.org/">
<img src="https://img.shields.io/badge/language-C%2B%2B-00599C?style=flat&logo=cplusplus&logoColor=white" alt="C++"></a>
<a href="https://www.qt.io/">
<img src="https://img.shields.io/badge/framework-Qt_6-41CD52?style=flat&logo=qt&logoColor=white" alt="Qt 6"></a>
<a href="https://github.com/foolspec/IntelGram/blob/main/CONTRIBUTING.md">
<img src="https://img.shields.io/badge/PRs-welcome-brightgreen?style=flat" alt="PRs welcome"></a>
</div>

<div align="center">
<a href="#installation">Installation</a> ·
<a href="#local-stars">Local Stars</a> ·
<a href="#local-profile">Local Profile</a> ·
<a href="#appearance">Appearance</a> ·
<a href="#faq">FAQ</a> ·
<a href="FEATURE_GUIDE.md">Docs</a> ·
<a href="#powered-by">Powered By</a>
</div>

Imagine opening Telegram and your profile, your Stars balance, and the entire look of the app are exactly how you want them — without changing anything about your account. IntelGram is a Telegram Desktop build, based on AyuGram, focused on local profile customization, local Stars, and private on-device tools. Everything it adds runs on your device only.

New in v21: local Stars moved into Telegram's native My Stars screen, and top-up runs through Telegram's real 15 Stars packages with a native checkout panel. See the [changelog](CHANGELOG.md).

## Installation

[![Release](https://img.shields.io/github/v/release/foolspec/IntelGram?display_name=tag&style=flat&color=2ea44f)](https://github.com/foolspec/IntelGram/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/foolspec/IntelGram/total?style=flat&color=fb9233)](https://github.com/foolspec/IntelGram/releases)

[latest release](https://github.com/foolspec/IntelGram/releases/latest) · macOS: Apple Silicon .dmg · Windows: x64 .zip · Linux: x64 .tar.gz · [all releases](https://github.com/foolspec/IntelGram/releases)

1. Download the build for your platform using the buttons at the top of this page.

2. macOS: open the DMG and drag `IntelGram.app` to Applications. Windows: extract the ZIP and run `IntelGram.exe`. Linux: extract the tarball and run `IntelGram`.

3. Open **IntelGram Preferences** and explore — or start with [Local Profile](#local-profile) and [Disguise Mode](#disguise-mode).

*macOS first launch: Control-click **Open** if Gatekeeper asks. The community build is ad-hoc signed, not Apple-notarized.*

## Local Stars

Your on-device Stars balance lives on Telegram's native **My Stars** row in Settings. **Top Up Balance** opens Telegram's own package list, and paying credits your local balance with the usual success animation.

<details><summary>How top-up works</summary>

**Top Up Balance** lists the real 15 Stars packages Telegram offers, from 100 to 150,000 Stars. Picking one opens Telegram's native checkout panel with the demo American Express card already saved as the payment method. The card carries an explicit **DEMO - AMEX - NO REAL PAYMENT** label and an editable cardholder name. Pay adds the Stars to your on-device balance only.

</details>

<details><summary>What it never does</summary>

No real card details are ever asked for. Nothing is sent to Telegram or a payment provider, and Telegram's payment APIs are never contacted. The balance exists only inside this IntelGram installation.

</details>

## Local Profile

Open **IntelGram Preferences -> Local Profile & Collectibles** to reshape how your own profile renders inside IntelGram. Every field has its own switch, so you can use only the pieces you want.

#### 👤 Profile fields

<details><summary>Every field you can override</summary>

Set a local display name, UID, primary username and up to 20 other usernames, an anonymous number, bio, and profile photo from a file on your device. The username editor visually matches Telegram's native Username dialog, including the `@username` field, validation row, and buttons — but it never asks Telegram whether a name is available and never updates the account. Each value can be cleared individually to return to the real Telegram-rendered value, and everything survives a restart.

</details>

#### 👥 Clone a profile

<details><summary>How cloning works</summary>

First open the other user's profile so IntelGram has loaded it, then choose **Clone profile locally by UID** and paste their UID. IntelGram refreshes that already-known profile read-only and mirrors only data visible to you: name, UID, usernames, visible phone, bio, photo, profile colors, Premium, verified, SCAM, FAKE, DIRECT, organization verification, emoji status, personal channel, and featured collectible. If the source has none of those elements, the cloned view shows none. Choose **Stop cloning profile** to return to your own local fields.

</details>

#### ✅ Badges

<details><summary>Badges and emoji status</summary>

Badges default to **Automatic**, which follows your clone profile or your real Telegram badge state. You can instead hide the primary badge or select Premium, verified, SCAM, FAKE, or DIRECT. Optional document-ID fields use Telegram's native custom-emoji renderer for an emoji status and an organization-verification symbol. Badges update everywhere they render: profile headers, chat headers, dialog rows, and search results.

</details>

#### 🎭 Disguise Mode

<details><summary>How to switch</summary>

Triple-click anywhere on the blue **IntelGram Desktop v...** heading in IntelGram Preferences within 1.2 seconds and the app's visible identity switches to Telegram Desktop: app name, logo, Dock or taskbar icon, tray icon, window title, menus, About surface, and drawer footer. The page returns to Telegram's normal Settings screen immediately, so IntelGram Preferences is no longer visible. On macOS the running app and Dock entry are titled exactly **Telegram**, with the full rounded-square Telegram artwork. Your local profile, collectibles, and badges stay active. Triple-click the **Telegram Desktop** drawer footer to come back. The choice is saved locally.

</details>

#### 🎁 Collectibles

<details><summary>How to browse</summary>

Choose **Choose featured collectible** or **Manage pinned collectible gifts** to open a native gallery of collection artwork from Telegram's live star-gift catalog. Click a collection to scroll its exact numbered gifts, and click the artwork to select one — the whole flow stays inside IntelGram. You can also paste a Telegram gift slug, `t.me/nft` link, Getgems item URL, or TON NFT address. Feature one collectible as the local profile backdrop and pin up to six around the local avatar. Ownership, sender, date, and transaction data are never changed.

</details>

#### 📢 Local Channels

<details><summary>Local ownership in a real channel</summary>

Open any broadcast channel, open the three-dot menu, and choose **IntelGram -> Local ownership**. The same channel screen now shows Telegram's native **Broadcast a message...** composer and owner-style **Manage channel** actions. Type a text post and send it normally; IntelGram intercepts it and inserts it into that channel's timeline as a persistent local message, with the native reaction strip and Reply, Edit, Pin, Copy, Delete, and Select controls. The **Broadcast engagement** editor sets starting and maximum views, increment size and interval, baseline reactions, and a paid-Star display count. Media, voice, stickers, and scheduled posts stay blocked so nothing can upload.

</details>

<details><summary>Local channel workspace</summary>

Click the **Local showcase** row on your profile or choose **Open local channel** in Local Profile settings. The private **Local-only channel** workspace keeps up to eight selected public channels, each with local text posts, optional local-file attachments, and its own title, description, and photo. Posts support editing, pinning, copying, and deleting. Nothing is published to the public channel.

</details>

#### 🔍 Find by UID or phone

<details><summary>How lookup works</summary>

Paste a numeric UID, `id: UID`, or a visible phone number into the normal chat-list search field and a matching profile appears under **Found by ID or phone**. Lookup is limited to peers already loaded by IntelGram and phone numbers already visible to your account, and it bypasses remote message, topic, and username search. No contact import or address-book sync is added.

</details>

## Appearance

#### ✨ Liquid Glass

<details><summary>What you can tune</summary>

Turn on Liquid Glass in **IntelGram Preferences -> Glass & Motion** to layer adaptive tint and contrast, wallpaper-colored diffusion, cursor-reactive highlights, refractive edges, and fine grain over native desktop blur — or a removable local whole-app image. Chat list, message surfaces, and menus can be softened independently, and text, icons, avatars, and media stay fully opaque. The slow decorative layer refreshes at a paced 30 Hz and pauses while hidden, inactive, or when reduced motion is enabled, so scrolling stays smooth.

</details>

#### 🎬 Motion

<details><summary>The five presets</summary>

Enable Enhanced Animations, then choose a pack: **Quick Snap** is compact and immediate; **Smooth Flow** is calm and critically damped; **Diabolical** is brisk with stronger depth; **Springy** settles gently without overshoot; **Liquid Bounce** is the most elastic, with a controlled overshoot. Presets apply across transitions, dialogs, drawers, and navigation, while reduced-motion and power-saving behavior is preserved.

</details>

#### 🎨 Themes and icons

<details><summary>Themes, backgrounds, and icons</summary>

Browse Telegram's native light and dark theme cards with visual previews, browse cloud themes inside IntelGram, and import `.tdesktop-theme`, `.tdesktop-palette`, or image files through the native file picker. Pick any local image as the chat background with a live preview, tiling, and wide-layout controls. The in-app icon picker offers the pink character icon, its profile-art variant, or twelve color variants.

<p>
  <img src="branding/icons/character/masters/intelgram-pink-front.png" alt="Pink character" width="32" />
  <img src="branding/icons/character/masters/intelgram-pink-profile.png" alt="Pink profile" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-01-midnight-purple.png" alt="Midnight purple" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-02-telegram-blue.png" alt="Telegram blue" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-03-deep-navy.png" alt="Deep navy" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-04-cyan-teal.png" alt="Cyan teal" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-05-indigo.png" alt="Indigo" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-06-black-green.png" alt="Black green" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-07-crimson.png" alt="Crimson" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-08-coral.png" alt="Coral" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-09-slate-blue.png" alt="Slate blue" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-10-lilac.png" alt="Lilac" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-11-white-blue.png" alt="White blue" width="32" />
  <img src="branding/icons/color/Linux/hicolor/256x256/apps/intelgram-square-12-graphite.png" alt="Graphite" width="32" />
</p>

</details>

## Vault and Search

Search everything IntelGram has already received, across every signed-in account, without anything leaving your device.

#### 🔎 Search and timeline

<details><summary>What's indexed</summary>

A private SQLite index with full-text search covers received message text, filenames, media types, and links. Search the current account, all accounts, or the unified inbox with clear account labels. Jump to a date in any chat, view conversation statistics, browse compact media history and locally observed edit and deletion history, and save private moments with a title, note, and tags. Protected messages appear only as metadata and jump-back references.

</details>

#### 🗂️ Smart folders and rules

<details><summary>How rules work</summary>

Smart folders filter your chats locally: **Unread from real people**, **Work**, **High priority**, **Needs reply**, and **Spam review**. Rules match a keyword, link, photo, or file on newly indexed messages and can tag, save a moment, mute locally, or send the item to spam review. Unknown senders and suspicious invite links can be routed to local review. Forward queues always open Telegram's native confirmation picker, and protected content can never enter one.

</details>

#### 📝 Private contact notes

<details><summary>Notes, reminders, and snapshots</summary>

Attach private notes, tags, relationship text, and an optional reminder date to any loaded contact, with a local history of note changes. Due reminders show on the **Contacts & Automation** page. The opt-in identity inspector snapshots only public fields already visible to IntelGram: name, usernames, badge flags, and shared-group count.

</details>

#### 💬 Per-chat controls

<details><summary>What you can set per chat</summary>

Set private tags and priority, download mode (Telegram default, manual, Wi-Fi only, or always), a read reminder, a local-only draft preference that prevents that chat's draft from being uploaded, and a local notification mute that never touches Telegram's server-side notification settings.

</details>

## Exports and Backup

#### 📤 Export formats

<details><summary>What an export contains</summary>

Export selected messages, a chat, or an account to HTML, PDF, Markdown, JSON, or ZIP. Account exports include contacts already visible to the account, profile notes and history, identity snapshots, rules and activity, smart-folder policies, moments, and revisions — without exposing host filesystem paths in JSON. Telegram's protected-content and self-destruct restrictions stay enforced throughout.

</details>

#### 🔒 Frozen Account Backup

<details><summary>How the encrypted archive works</summary>

Frozen Account Backup streams the full structured export and permitted downloaded files into one `.intelvault` archive, encrypted with AES-256-GCM and a passphrase you enter (PBKDF2-HMAC-SHA256, 250,000 iterations). No plaintext export files are left behind, and Telegram login keys, session credentials, protected media, and self-destructing content are deliberately excluded.

</details>

## Organized Preferences

IntelGram Preferences groups everything into **Customize**, **Power tools**, and **Client settings**: Local Profile & Collectibles, Appearance & Backgrounds, Glass & Motion, Navigation & Layout, Vault & Search, Contacts & Automation, Export & Backup, and Advanced & Maintenance.

## Local means local

These controls only change how your own profile is rendered inside this IntelGram installation. They do not change your Telegram display name, photo, bio, username, UID, or collectible ownership, and other Telegram users do not see the local overrides.

## FAQ

<details><summary>👁️ Can anyone else see my local profile?</summary>

No. The overrides are visible only in this IntelGram installation. Screenshots will naturally show what IntelGram rendered, but Telegram and other users receive none of these local values.

</details>

<details><summary>⭐ Is the Stars top-up a real payment?</summary>

No. It is a local-only simulation: paying credits your on-device balance, with no real money, no payment provider, and no Telegram payment API involved.

</details>

<details><summary>🎭 How do I turn on the hidden Telegram mode?</summary>

Open **IntelGram Preferences** and click the blue **IntelGram Desktop v...** heading three times within 1.2 seconds. The app name, icon, window title, menus, and About surface switch to Telegram Desktop. To switch back, open the main drawer and click **Telegram Desktop** at the bottom three times. The choice is saved on this device only.

</details>

<details><summary>🆓 Is IntelGram free and open source?</summary>

Yes. IntelGram is free, and the source is open under the GNU General Public License v3.0. See [LICENSE](LICENSE).

</details>

<details><summary>💻 Does IntelGram replace Telegram or AyuGram?</summary>

No. IntelGram uses its own macOS bundle ID, Windows application ID, and Linux desktop ID, so it coexists with a normal AyuGram installation without overwriting it.

</details>

<details><summary>📝 Where do I report a bug or ask a question?</summary>

Open an issue on [GitHub](https://github.com/foolspec/IntelGram/issues), ask in [Discussions](https://github.com/foolspec/IntelGram/discussions), or join [@intelgrams](https://t.me/intelgrams) on Telegram.

</details>

More questions are answered in [Discussions](https://github.com/foolspec/IntelGram/discussions).

## Building

IntelGram is built from the official AyuGram Desktop `v6.7.8` source, with the complete implementation in [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch). Every release includes SHA-256 checksums and platform validation notes. The [feature guide](FEATURE_GUIDE.md) walks through every workflow, and [FEATURES.md](FEATURES.md) documents the full feature surface.

## Powered By

[![AyuGram](https://img.shields.io/badge/AyuGram-v6.7.8-229ED9?style=flat&logo=telegram&logoColor=white)](https://github.com/AyuGram/AyuGramDesktop)
[![Telegram Desktop](https://img.shields.io/badge/Telegram_Desktop-upstream-0088cc?style=flat)](https://github.com/telegramdesktop/tdesktop)
[![Qt](https://img.shields.io/badge/Qt-6.2.13-41CD52?style=flat&logo=qt&logoColor=white)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-build-064F8C?style=flat&logo=cmake&logoColor=white)](https://cmake.org/)
[![Breakpad](https://img.shields.io/badge/Breakpad-crash_reporting-9E9E9E?style=flat)](https://chromium.googlesource.com/breakpad/breakpad/)

| Service | Purpose |
| ------- | ------- |
| AyuGram Desktop (v6.7.8) | The Telegram client IntelGram is built on |
| Telegram Desktop | The upstream open-source client |
| Qt 6 (6.2.13) | The UI framework |
| C++ | The implementation language |
| CMake | The build system |
| Breakpad | Crash reporting |

## Credits

IntelGram is based on [AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop), which is based on [Telegram Desktop](https://github.com/telegramdesktop/tdesktop). Their upstream licenses and attribution remain in the source.

IntelGram custom features by **fool**.
