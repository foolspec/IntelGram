# IntelGram

IntelGram is a cross-platform AyuGram Desktop build focused on local profile customization, private on-device productivity tools, and verifiable local exports.

[![IntelGram release validation](https://github.com/foolspec/IntelGram/actions/workflows/intelgram-release-validation.yml/badge.svg?branch=main)](https://github.com/foolspec/IntelGram/actions/workflows/intelgram-release-validation.yml)

[All features](FEATURES.md) | [Feature guide](FEATURE_GUIDE.md) | [Changelog](CHANGELOG.md) | [Technical changelog](TECHNICAL_CHANGELOG.md) | [Update log](UPDATE_LOG.md)

## Main Features

- Search messages, media metadata, links, and filenames already received by IntelGram with an on-device full-text vault and a unified inbox across signed-in accounts.
- Use jump-to-date, conversation statistics, compact media history, saved moments, and locally observed message edit/deletion history.
- Organize work with smart folders, private contact notes and reminders, opt-in public identity snapshots, local rules, and anti-spam review.
- Set per-chat tags, priority, download behavior, read reminders, local-only draft preference, and local notification muting.
- Export selected messages, a chat, or an account to HTML, PDF, Markdown, JSON, or ZIP; create an AES-256-GCM encrypted Frozen Account Backup with permitted cached media.
- Switch among Windows 93, Terminal, Classic Telegram, AMOLED, imported theme packs, and IntelGram icon choices.
- Render a local display name, UID, primary username, up to 20 other usernames, anonymous number, bio, and profile photo for your own account.
- Clone the visible profile presentation of a user already loaded in IntelGram by entering their UID, including premium or verification badges, organization badge symbols, emoji status, and personal channel.
- Find an already-loaded user by UID or by a phone number that is visible to your account, directly from the normal chat-list search field.
- Browse every collection reported by Telegram's live collectible catalog, inspect exact numbered gifts in a scrollable native grid, and paste Telegram, Getgems, or TON item links.
- Feature one collectible as the local profile backdrop and pin up to six around the local avatar.
- Open locally featured and cloned gift details with your currently rendered display name in both the recipient link and Telegram profile chip; the compact profile card also uses your local photo, phone, usernames, bio, and personal channel.
- Show your active local or cloned display name beside the local avatar in collectible username and anonymous-number owner chips.
- Work through grouped **Identity**, **Usernames, bio and contact**, **Profile photo**, and **Collectibles** settings instead of one long control list.
- Keep the name-color preview synchronized with the locally rendered display name.
- Choose the pink IntelGram icon, its profile-art variant, or any of twelve supplied color variants from the in-app icon picker.
- Read the latest changes and main feature summary from a bundled update log inside IntelGram.
- Open the IntelGram community and project links directly from settings, with an optional local supporter badge after you join `@intelgrams` yourself.
- Preserve Telegram's Restrict Saving Content and self-destruct rules: protected bodies and media never enter the vault, rules, exports, or backups.
- Keep every profile override client-render-only, with no Telegram profile, ownership, contact, channel-join, or account mutation.

## Downloads

| Platform | Package |
| --- | --- |
| macOS Apple Silicon | [IntelGram DMG](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-macOS-Apple-Silicon.dmg) |
| Windows x64 | [IntelGram ZIP](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Windows-x64.zip) |
| Linux x64 | [IntelGram tar.gz](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Linux-x64.tar.gz) |

[View the latest release, checksums, and validation logs](https://github.com/foolspec/IntelGram/releases/latest).

## Install

- macOS: open the DMG, drag `IntelGram.app` to Applications, then Control-click **Open** on first launch if Gatekeeper asks. The community build is ad-hoc signed, not Apple-notarized.
- Windows: extract `IntelGram-Windows-x64.zip` and run `IntelGram.exe`. Keep the files from the ZIP together.
- Linux: extract `IntelGram-Linux-x64.tar.gz` and run `IntelGram`.

## Using IntelGram

Open **IntelGram Settings -> Other -> Local profile** to configure:

- Local display name
- Local UID
- Local primary username
- Up to 20 local usernames
- Local anonymous number
- Local bio and profile photo
- Local profile cloning by the UID of a user already opened in IntelGram
- A featured collectible gift and up to six pinned collectible gifts

In the main chat-list search field, paste a UID, `id: UID`, or a visible phone number. IntelGram checks only profiles already loaded in this client and shows the matching profile row under **Found by ID or phone**. Phone and UID lookup does not import contacts or send a profile lookup request.

Clicking your username in IntelGram's profile settings opens the local username editor. The familiar inline Telegram-style status validates local syntax and shows the local value as available without sending a username check or save request to Telegram.

The collectible picker opens as a native visual collection gallery. Choose a collection card to open a scrollable grid of its exact numbered collectibles, click any artwork to select it without leaving IntelGram, or paste a supported Telegram gift slug, `t.me/nft` link, Getgems item URL, or TON NFT address. IntelGram resolves the collectible read-only, shows its native `Collection #Number` profile tooltip, and uses Telegram's native collectible detail view when clicked. For a locally selected or cloned gift, the detail view presents both the recipient link and Telegram profile chip as your real or enabled local display name, with both opening your own locally rendered profile. The compact profile card uses the active local photo, phone, usernames, bio, and personal channel; the gift's actual ownership, sender, date, and transaction data are not changed.

The local-profile page is split into focused identity, contact, photo, and collectible groups. **Settings -> Appearance -> App Icon** includes the new pink IntelGram artwork, a profile-art alternate, and twelve coordinated color variants. macOS uses full-bleed platform artwork without a duplicate white frame or inset. The name-color editor uses your currently rendered local display name in its preview.

Choose a local image to replace your own profile photo throughout this IntelGram installation. Profile cloning accepts the UID of a user whose profile has already been opened and locally mirrors their visible name, UID, usernames, phone, bio, photo, profile colors, premium or verification badges, organization badge symbol, emoji status, personal channel, and featured collectible. After selection, IntelGram performs Telegram's standard read-only full-profile refresh for that already-known user so visible badge and personal-channel metadata can render immediately. If the source profile lacks a badge, status, or personal channel, IntelGram clears that element from the cloned local view. Stop cloning at any time to return to the individual local fields.

The IntelGram settings page links to [`@intelgrams`](https://t.me/intelgrams), this GitHub repository, and both changelogs. Joining the channel is an explicit Telegram action: open the channel and press Telegram's normal **Join** button. IntelGram then derives the supporter badge from the locally known membership state; it never joins a channel in the background.

Open **IntelGram Settings -> IntelGram -> Update log** to read the latest update, main IntelGram features, and local-only privacy boundary without leaving the app. The dialog also provides an optional link to the complete GitHub changelog.

Open **IntelGram Settings -> Vault & Tools** for:

- **Search & timeline:** current-account or all-account vault search, unified inbox, smart folders, jump-to-date, statistics, media history, revisions, and saved moments.
- **Contacts & automation:** private notes, tags, relationship context, reminders, opt-in identity history, configurable local rules, and rule activity.
- **Current chat tools:** local tags and priority, manual/Wi-Fi/always download mode, read reminder, local-only draft preference, and local notification mute.
- **Themes & backup:** built-in and imported themes, normal local exports, and encrypted Frozen Account Backup.

Select one or more ordinary messages and use **Export selected messages** from the context menu to create a scoped export. In a protected chat, the same action records only basic metadata and a jump-back reference.

## Local Means Local

These controls only change how your own profile is rendered inside this IntelGram installation. They do not change your Telegram display name, photo, bio, username, UID, phone number, emoji status, collectible ownership, or profile data. Other Telegram users do not see the local overrides.

The vault processes content this client has already received. It does not fetch hidden history or bypass Telegram restrictions. Restrict Saving Content and self-destructing media are never copied into search text, rule payloads, media archives, or backups; only a local reference remains so you can return to the original message.

The implementation contains no Telegram account/profile mutation request, contact import, automatic channel join, or collectible transaction. Clone metadata refresh and collectible catalog/detail requests are read-only. Normal Telegram account editing remains available whenever the corresponding IntelGram local override is disabled.

## Platform Builds

All packages are built from the official AyuGram Desktop `v6.7.8` source at commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with its required submodules.

- macOS: Apple Silicon `.app`, ad-hoc signed, launch tested, packaged as DMG and ZIP
- Windows: x64 `IntelGram.exe`, launch tested, packaged as ZIP
- Linux: x64 `IntelGram`, launch tested under Xvfb, packaged as tar.gz

IntelGram uses its own visible application name, macOS bundle ID, Windows application ID, and Linux desktop ID so it can coexist with a normal AyuGram installation.

## Source And Verification

- [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch) contains the complete IntelGram implementation.
- Patch SHA-256: `566f49ca3979c62366bb8ebb4543d3cc7694037aa5781eaa4db031ec1b78b94d`
- [`build_intelgram_branding.py`](build_intelgram_branding.py) applies the cross-platform IntelGram product identity.
- [`validate_intelgram_patch.py`](validate_intelgram_patch.py) verifies the feature hooks, protected-content boundaries, and absence of custom Telegram mutation requests before each platform build.
- [`branding/icons`](branding/icons) contains the pink character artwork and twelve color masters with generated macOS, Windows, and Linux resources; [`generate_intelgram_character_icons.py`](generate_intelgram_character_icons.py) reproduces them.
- [`.github/workflows/intelgram-multiplatform-build.yml`](.github/workflows/intelgram-multiplatform-build.yml) performs clean macOS, Windows, and Linux builds.
- Every release includes SHA-256 checksums, platform validation notes, and launch logs.
- [`FEATURES.md`](FEATURES.md) documents the complete custom feature surface and render coverage.
- [`FEATURE_GUIDE.md`](FEATURE_GUIDE.md) is a conversational walkthrough of the common workflows.
- [`CHANGELOG.md`](CHANGELOG.md), [`TECHNICAL_CHANGELOG.md`](TECHNICAL_CHANGELOG.md), and [`UPDATE_LOG.md`](UPDATE_LOG.md) track product, implementation, and build changes.

IntelGram preserves AyuGram's internal settings keys and source namespaces for compatibility. Product-facing names and package identities are IntelGram.

## Credits

IntelGram is based on [AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop), which is based on [Telegram Desktop](https://github.com/telegramdesktop/tdesktop). Their upstream licenses and attribution remain in the source.

IntelGram custom features by **fool**.
