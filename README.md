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
- Browse visual theme cards and cloud themes inside IntelGram, import or remove custom themes, choose any local image as the chat background, preview wallpaper changes, and select IntelGram app icons.
- Turn on Liquid Glass with cached native or local-image blur, adaptive tint and contrast, wallpaper-colored diffusion, moving specular highlights, edge refraction, subtle grain, and an optional removable local background image.
- Make the interface more lively with Quick Snap, Smooth Flow, Diabolical, Springy, or the optional high-elasticity Liquid Bounce preset while preserving reduced-motion and power-saving behavior.
- Render a local display name, UID, primary username, up to 20 other usernames, anonymous number, bio, profile photo, and native Telegram badge presentation for your own account.
- Show a public channel on your own profile as a clearly labeled local showcase, then open its private local-only workspace to create, edit, pin, copy, or delete local posts, attach local files, and customize a local title, description, and photo without changing Telegram.
- Enable **Local ownership** from any broadcast channel's **IntelGram** submenu to reveal the native broadcast composer and Manage actions, then place persistent local text broadcasts directly in that channel's real timeline with native reply, edit, pin, reaction, copy, delete, and selection controls without sending them to Telegram.
- Clone the visible profile presentation of a user already loaded in IntelGram by entering their UID, including Premium, verified, SCAM, FAKE, DIRECT, emoji-status, organization-verification, and personal-channel presentation.
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

Open **IntelGram Preferences -> Local Profile & Collectibles** to configure:

- Local display name
- Local UID
- Local primary username
- Up to 20 local usernames
- Local anonymous number
- Local bio and profile photo
- Automatic or manually selected local profile badges, emoji status, and organization verification icon
- Local profile cloning by the UID of a user already opened in IntelGram
- An optional public-channel showcase rendered only on your local profile, with a private local posting workspace
- A featured collectible gift and up to six pinned collectible gifts

In the main chat-list search field, paste a UID, `id: UID`, or a visible phone number. IntelGram checks only profiles already loaded in this client and shows the matching profile row under **Found by ID or phone**. Phone and UID lookup does not import contacts or send a profile lookup request.

Clicking your username opens an editor that matches Telegram's native **Username** dialog: the same `@username` field, inline validation row, explanatory copy, spacing, and Save/Cancel actions. Saving still changes only IntelGram's local render value and sends no username check or account-update request to Telegram.

The collectible picker opens as a native visual collection gallery. Choose a collection card to open a scrollable grid of its exact numbered collectibles, click any artwork to select it without leaving IntelGram, or paste a supported Telegram gift slug, `t.me/nft` link, Getgems item URL, or TON NFT address. IntelGram resolves the collectible read-only, shows its native `Collection #Number` profile tooltip, and uses Telegram's native collectible detail view when clicked. For a locally selected or cloned gift, the detail view presents both the recipient link and Telegram profile chip as your real or enabled local display name, with both opening your own locally rendered profile. The compact profile card uses the active local photo, phone, usernames, bio, and personal channel; the gift's actual ownership, sender, date, and transaction data are not changed.

The local-profile page is split into focused identity, contact, photo, and collectible groups. **IntelGram Preferences -> Appearance & Backgrounds** contains visual theme cards, the in-app cloud theme gallery, a visible action for removing the selected custom theme, a live chat-background preview, Telegram's wallpaper gallery, local-image selection, tiling controls, and the IntelGram app-icon picker. **Navigation & Layout** separately contains chat-folder, tray, and drawer controls. macOS uses full-bleed platform artwork without a duplicate white frame or inset. The name-color editor uses your currently rendered local display name in its preview.

Open **IntelGram Preferences -> Glass & Motion** for the optical-material controls. Liquid Glass can independently soften the chat list/sidebar, message surfaces, and menus/dialogs, then layer adaptive tint and contrast, wallpaper-colored diffusion, cursor-reactive highlights, moving reflections, refractive edges, and fine anti-banding grain over native desktop blur or a removable local whole-app image. The backdrop and scaled-cover image are cached, and active material motion follows the display refresh rate up to 120 Hz while pausing when the window is hidden, inactive, or reduced motion is enabled.

Enhanced Animations applies one selected pack across existing IntelGram transitions, dialogs, drawers, controls, and navigation. **Quick Snap** is compact and immediate, **Smooth Flow** is calm and critically damped, **Diabolical** is brisk with stronger depth, **Springy** settles gently without overshoot, and **Liquid Bounce** adds the most elastic lift with a controlled overshoot. The optional window-opening fade follows the same accessibility gate. Turning either master switch off restores normal IntelGram appearance or motion immediately.

Choose a local image to replace your own profile photo throughout this IntelGram installation. Open **Profile badges** to leave badge rendering on **Automatic**, hide the primary badge, or locally select Premium, verified, SCAM, FAKE, or DIRECT. Optional Telegram document IDs render a custom emoji status and organization-verification symbol using Telegram's native badge artwork.

Profile cloning accepts the UID of a user whose profile has already been opened and locally mirrors their visible name, UID, usernames, phone, bio, photo, profile colors, every supported primary badge, organization-verification symbol, emoji status, personal channel, and featured collectible. After selection, IntelGram performs Telegram's standard read-only full-profile refresh for that already-known user so visible badge and personal-channel metadata can render immediately. If the source profile lacks a badge, status, or personal channel, IntelGram clears that element from the cloned local view. Stop cloning at any time to return to the individual local fields.

The local channel showcase accepts a public `@channel` or `t.me` link, resolves that public channel through Telegram's normal read-only lookup, and renders it only on your own profile in this IntelGram installation. Click the showcased channel or choose **Open local channel** to enter its local workspace. You can create text posts, attach local files, edit or delete posts, pin highlights, copy text, open local attachments, and configure a local title, description, and photo. Workspaces are stored separately for up to eight selected channels and remain available after restarting IntelGram.

The workspace is deliberately labeled **Local-only channel** and the profile row remains labeled **Local showcase**. It does not make you the owner or an administrator, publish a Telegram message, join the channel, change its public information, create invites, grant permissions, or alter what anybody else sees.

For a native owner-style view of an existing broadcast channel, open that channel's three-dot menu, choose **IntelGram -> Local ownership**, and enable the switch. IntelGram exposes Telegram's **Broadcast a message...** composer, owner-style **Manage channel** actions, and persistent local text broadcasts in the channel's actual timeline. Join, Leave, and Report rows are suppressed while the mode is active. Right-clicking a local broadcast opens the native reaction strip and owner-style Reply, Edit, Pin/Unpin, Copy Text, Copy Post Link, Forward, Delete, and Select actions. Replies, edits, pins, reaction counts, and local links persist across restarts.

The ownership panel's **Broadcast engagement** editor sets starting views, a maximum, the increment size, the interval, six baseline reaction counts, and a paid-Star display count. Views rise locally until the selected maximum, while clicks in the native reaction strip increment only the selected local counter. No view, reaction, payment, pin, edit, delete, or post request is sent to Telegram.

This native mode is separate from the profile showcase workspace. Text posts are intercepted before Telegram's send path and stored in IntelGram's local vault. Media, files, voice messages, stickers, inline results, direct forwarding of synthetic message IDs, and scheduled posts are blocked so they cannot upload or mutate the channel. The local **Forward** row copies the post into a safe text handoff instead of submitting the synthetic record to Telegram. Turning the switch off removes the injected local posts and restores the ordinary subscriber view; turning it back on restores the locally stored posts.

The IntelGram settings page links to [`@intelgrams`](https://t.me/intelgrams), this GitHub repository, and both changelogs. Joining the channel is an explicit Telegram action: open the channel and press Telegram's normal **Join** button. IntelGram then derives the supporter badge from the locally known membership state; it never joins a channel in the background.

Open **IntelGram Preferences -> Update log** to read the latest update, main IntelGram features, and local-only privacy boundary without leaving the app. The dialog also provides an optional link to the complete GitHub changelog.

IntelGram Preferences is grouped into **Customize**, **Power tools**, and **Client settings**:

- **Vault & Search:** current-account or all-account vault search, unified inbox, smart folders, jump-to-date, statistics, media history, revisions, and saved moments.
- **Contacts & Automation:** private notes, tags, relationship context, reminders, opt-in identity history, configurable local rules, rule activity, and privacy controls.
- **Export & Backup:** normal local exports and encrypted Frozen Account Backup.
- **Appearance & Backgrounds:** native theme previews, cloud themes, custom theme import/removal, wallpaper gallery, local background images, live preview, and app icons.
- **Glass & Motion:** adaptive Liquid Glass, native blur, local window backdrops, optical-material controls, and five global motion presets.
- **Advanced & Maintenance:** diagnostics, URL registration, and resetting IntelGram settings.

Select one or more ordinary messages and use **Export selected messages** from the context menu to create a scoped export. In a protected chat, the same action records only basic metadata and a jump-back reference.

## Local Means Local

These controls only change how your own profile is rendered inside this IntelGram installation. They do not change your Telegram display name, photo, bio, username, UID, phone number, emoji status, collectible ownership, or profile data. Other Telegram users do not see the local overrides.

The vault processes content this client has already received. It does not fetch hidden history or bypass Telegram restrictions. Restrict Saving Content and self-destructing media are never copied into search text, rule payloads, media archives, or backups; only a local reference remains so you can return to the original message.

The implementation contains no Telegram account/profile mutation request, contact import, automatic channel join, server-side channel ownership or permission mutation, or collectible transaction. Local ownership changes only this client's controls and local history; it does not set creator/admin flags or submit its broadcasts, views, reactions, Stars, pins, or edits. Clone metadata refresh, public-channel showcase lookup, and collectible catalog/detail requests are read-only. Normal Telegram account editing remains available whenever the corresponding IntelGram local override is disabled.

## Platform Builds

All packages are built from the official AyuGram Desktop `v6.7.8` source at commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with its required submodules.

- macOS: Apple Silicon `.app`, ad-hoc signed, launch tested, packaged as DMG and ZIP
- Windows: x64 `IntelGram.exe`, launch tested, packaged as ZIP
- Linux: x64 `IntelGram`, launch tested under Xvfb, packaged as tar.gz

IntelGram uses its own visible application name, macOS bundle ID, Windows application ID, and Linux desktop ID so it can coexist with a normal AyuGram installation.
Liquid Glass uses a native behind-window material on macOS, the Windows DWM backdrop where available, and the desktop compositor's ARGB transparency on Linux. A cached local-image fallback is available on every platform.

## Source And Verification

- [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch) contains the complete IntelGram implementation.
- Patch SHA-256: `a91aa6db95d219be7443fc4db3e07c68b0b7ba39648f438af2a2c2e51d3618f5`
- The patch pins [`foolspec/lib_ui`](https://github.com/foolspec/lib_ui) commit `b9a30917daf2bd8fdc17ccd9682acca178882b7b`, which carries IntelGram's shared motion-preset hook and Liquid Bounce transition.
- [`build_intelgram_branding.py`](build_intelgram_branding.py) applies the cross-platform IntelGram product identity.
- [`validate_intelgram_patch.py`](validate_intelgram_patch.py) verifies the feature hooks, protected-content boundaries, and absence of custom Telegram mutation requests before each platform build.
- [`branding/icons`](branding/icons) contains the pink character artwork and twelve color masters with generated macOS, Windows, and Linux resources; [`generate_intelgram_character_icons.py`](generate_intelgram_character_icons.py) reproduces them.
- [`.github/workflows/intelgram-multiplatform-build.yml`](.github/workflows/intelgram-multiplatform-build.yml) performs clean macOS, Windows, and Linux builds.
- Every release includes SHA-256 checksums, platform validation notes, and launch logs.
- [`FEATURES.md`](FEATURES.md) documents the complete custom feature surface and render coverage.
- [`FEATURE_GUIDE.md`](FEATURE_GUIDE.md) is a conversational walkthrough of the common workflows.
- [`CHANGED_FILES.md`](CHANGED_FILES.md) lists every source path changed by the complete patch.
- [`CHANGELOG.md`](CHANGELOG.md), [`TECHNICAL_CHANGELOG.md`](TECHNICAL_CHANGELOG.md), and [`UPDATE_LOG.md`](UPDATE_LOG.md) track product, implementation, and build changes.

IntelGram preserves AyuGram's internal settings keys and source namespaces for compatibility. Product-facing names and package identities are IntelGram.

## Credits

IntelGram is based on [AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop), which is based on [Telegram Desktop](https://github.com/telegramdesktop/tdesktop). Their upstream licenses and attribution remain in the source.

IntelGram custom features by **fool**.
