# IntelGram Technical Changelog

This file records implementation-level changes to IntelGram's custom layer. Product-facing changes are summarized in [`CHANGELOG.md`](CHANGELOG.md).

## IntelGram v6.7.8 Local Owner Controls And Profile Badges - 2026-07-27

### Source Baseline And Patch

- Upstream source: official AyuGram Desktop `v6.7.8`, commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with required submodules.
- Source commit: `fb6f3f7aec5be0375f9033471aec8996b969e920` on the recovered local implementation branch.
- Delivery patch: [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch).
- Compatibility alias: [`ayugram-local-profile-render-overrides.patch`](ayugram-local-profile-render-overrides.patch), byte-for-byte identical.
- Patch SHA-256: `a91aa6db95d219be7443fc4db3e07c68b0b7ba39648f438af2a2c2e51d3618f5`.
- Patch footprint: 108 files, 12,957 insertions, and 649 deletions relative to the pinned source.
- `Telegram/lib_ui` remains pinned to public fork commit `b9a30917daf2bd8fdc17ccd9682acca178882b7b`.

### Local Post Model

- The versioned `LocalBroadcast` record now persists a reply UID, pin state, bounded extra-reaction map, per-post view state, and the engagement snapshot used when the post was created.
- `Ayu::EnsureLocalChannelBroadcasts` rebuilds local reply references and pinned flags in chronological order, then registers restored pins through `HistoryItem::setIsPinned` so Telegram's local pinned tracker can display them.
- `Ayu::ShowEditLocalChannelBroadcast`, `ToggleLocalChannelBroadcastPinned`, `ConfirmDeleteLocalChannelBroadcast`, and `LocalChannelBroadcastLink` provide dedicated local mutations without calling Telegram channel methods.
- Local post links use the isolated `intelgram://local-channel/<account>/<channel>/<uid>` namespace and are copied only as client-local references.

### Native Context And Reactions

- `AyuUi::AddLocalChannelBroadcastActions` is shared by both `HistoryInner` and `HistoryView::FillContextMenuItems`, so the legacy and current history implementations route synthetic broadcasts into the same owner-style menu before ordinary permission and network actions are considered.
- The dedicated menu uses Telegram's existing localized Reply, Edit, Pin/Unpin, Copy Text, Copy Post Link, Forward, Delete, and Select rows and icons.
- `HistoryView::AttachSelectorToMenu` and `Data::LookupPossibleReactions` expose the normal reaction strip for local broadcasts even though those client-only messages are not server-reactable.
- `HistoryView::ListWidget::reactionChosen` and the favorite-reaction shortcut update the local vault record and animation directly. Paid reactions are intercepted before payment handling.
- Fixed reactions are stored in the engagement snapshot; other emoji and custom-emoji IDs are stored in a bounded map and rendered through native `MTPReactionCount` objects without sending them.
- The reaction aggregate is boxed as `MTPMessageReactions`, owner rows use the native `style_info` definitions, and the context menu imports Telegram's text utility directly for cross-platform compiler compatibility.

### Views, Replies, And Guards

- The engagement timer advances views at a configurable interval and step until the configured maximum, updates native message view rendering, and persists the result locally.
- Both current composer implementations pass the active local reply target into the intercepted broadcast path and clear the reply state after a successful local save.
- `Api::ViewsManager`, `Data::Reactions::send`, reaction polling, and paid-reaction sending return before all Telegram requests for synthetic local broadcasts.
- The menu's Forward row copies a text handoff rather than calling `ShowForwardMessagesBox`, so no local message ID can reach Telegram's forwarding API.
- No full local build is run; platform compilation, packaging, and launch testing remain delegated to the release workflows as required by the upstream repository instructions.

### Local Profile Badge Layer

- `AyuSettings` persists one local primary-badge mode plus optional emoji-status and organization-verification document IDs.
- Automatic mode resolves the selected clone user and mirrors Telegram's Premium, verified, SCAM, FAKE, DIRECT, emoji-status, and organization-verification state; missing clone state remains absent.
- `Info::Profile::BadgeValue`, `EmojiStatusIdValue`, `Ui::PeerBadge`, profile verification content, dialog search rows, and chat top bars share the local badge helpers.
- The settings page exposes one focused native radio-selector box and leaves Automatic selected by default.
- The validator requires the badge settings, native render hooks, clone-aware helper, and local-only copy while continuing to reject all Telegram profile and emoji-status mutation methods.

## IntelGram v6.7.8 Native Local Channel Ownership - 2026-07-26

### Source Baseline And Patch

- Upstream source: official AyuGram Desktop `v6.7.8`, commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with required submodules.
- Source commit: `337ba5d70afb6f2b8c012ea508143d7bb3d19d64` on the recovered local implementation branch.
- Delivery patch: [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch).
- Compatibility alias: [`ayugram-local-profile-render-overrides.patch`](ayugram-local-profile-render-overrides.patch), byte-for-byte identical.
- Patch SHA-256: `2a5b477398ed94120009acef684d4d6307f52928b57bf5b4a084a13bc29a933c`.
- Patch footprint: 85 files, 10,845 insertions, and 551 deletions relative to the pinned source.
- `Telegram/lib_ui` remains pinned to public fork commit `b9a30917daf2bd8fdc17ccd9682acca178882b7b`.

### Native Channel Integration

- `Ayu::ShowLocalChannelOwnership` is exposed from the existing IntelGram channel submenu and persists one enabled state plus up to 100 bounded text posts per account and broadcast peer.
- `HistoryWidget` and `HistoryView::ChatWidget` treat an enabled local channel as text-sendable, restore its local posts, expose the native composer, and intercept text before constructing or dispatching a Telegram send request.
- Local broadcasts use `History::addNewLocalMessage`, client-local message IDs, `MessageFlag::Post`, and native channel history rendering. The runtime registry connects those messages to their local vault record for deletion.
- The legacy and current history context menus expose only the dedicated local deletion path for injected broadcasts. Normal forwarding, server deletion, report, export, selection, and revision actions remain unavailable because the items are marked local by Telegram's own history constructor.
- `Window::PeerMenu` and `Info::Profile::TopBar` expose native **Manage channel** affordances while active, but route them to the local settings box instead of `EditPeerInfoBox`.
- Channel profile actions suppress Join, Leave, and Report while active and react to the local rights-refresh signal without changing `ChannelData` rights or membership state.
- `Ui::SilentToggle` accepts a local-only mode. It preserves the native bell control but skips `NotifySettings::update`, so clicking it cannot issue a Telegram settings mutation.

### Send Boundary

- Text-only interception occurs before `Api::MessageToSend` and before every Telegram message-send call.
- Attach picker, prepared-file confirmation, existing photo/document, voice, sticker, inline result, bot command, forward, and scheduled-send paths are blocked with a local-mode toast.
- Typing, sticker-selection progress, and cloud-draft updates are suppressed while local ownership is active.
- The feature adds no MTProto channel/account mutation, creator/admin flag, permission change, membership update, or Telegram history write.

### Validation

- The complete patch applies and reverses cleanly against the pinned source, passes whitespace checks, and keeps both patch aliases byte-identical.
- `validate_intelgram_patch.py` requires the local ownership storage, menu, history, composer, profile, and silent-toggle guards in addition to the existing IntelGram feature checks.

## IntelGram v6.7.8 Liquid Glass And Local Channels - 2026-07-26

### Source Baseline And Patch

- Upstream source: official AyuGram Desktop `v6.7.8`, commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with required submodules.
- Source commit: `9d0e4ac005c19eeceade7b13ef15eadeb489e73d` on the recovered local implementation branch.
- Delivery patch: [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch).
- Compatibility alias: [`ayugram-local-profile-render-overrides.patch`](ayugram-local-profile-render-overrides.patch), byte-for-byte identical.
- Patch SHA-256: `3481f2177ad41a055654b7a4c962d16073e70fbf0e12fe99013ebe56f88a3dc6`.
- Patch footprint: 77 files, 9,985 insertions, and 501 deletions relative to the pinned source.
- `Telegram/lib_ui` points to public fork [`foolspec/lib_ui`](https://github.com/foolspec/lib_ui), pinned at `b9a30917daf2bd8fdc17ccd9682acca178882b7b`.

### Optical Material Renderer

- `Window::MainWindow::paintEvent` keeps `Ayu::VisualEffects::PaintWindow` as the single behind-content render hook, so the material is painted before Qt child widgets and never fades text, controls, avatars, or media.
- `Ayu::VisualEffects` combines the native platform backdrop or cached local image with adaptive tint and contrast, backdrop-color diffusion, cursor- and time-reactive radial light, a moving specular sweep, a soft top reflection, deterministic 1-2% film grain, and anti-aliased inner and outer edge light.
- `WindowPath` and `EdgePath` model a rounded optical edge with an 8-pixel visual thickness. A local-image backdrop is sampled again inside that edge with a continuously animated 1-3 pixel displacement while the center remains undistorted.
- The material samples the current backdrop average and theme background luminance to adjust tint and highlight strength instead of using a fixed alpha overlay.
- `CoverImage` caches smooth scaled-cover results per window size. The decoded source and `Images::BlurLargeImage` result remain cached until the image path or blur radius changes.
- `MainWindow::_glassAnimationTimer` repaints at the active screen refresh interval, clamped from 30 through 120 Hz. It runs only for a visible active window with dynamic highlights enabled and stops under the existing `anim::Disabled()` accessibility and power-saving gate.

### Settings And Motion

- `AyuSettings` persists adaptive material, dynamic highlights, edge refraction, and optical-material intensity with conservative normalized defaults.
- `Settings::AyuVisualEffects` exposes the new controls and a bundled explanation for all five motion packs.
- The shared `lib_ui` motion enum adds `MotionStyle::LiquidBounce`. It scales duration to 140% and applies `bumpy(1.12)` for a controlled overshoot.
- Quick Snap remains the compact 75% quintic preset; Smooth Flow uses a 110% cubic settle; Diabolical uses a brisk 95% circular exit; Springy uses a monotonic 125% critically damped settle.

### Local Channel Showcase

- `ShowLocalProfileChannelBox` normalizes an `@channel` or `t.me` link, performs `MTPcontacts_ResolveUsername`, accepts only a public channel result, and requests its normal full peer read-only for rendering.
- `LocalProfileChannelShowcaseActive`, `LocalProfilePersonalChannelValue`, and `LocalProfilePersonalChannelMessageId` supply the selected channel only for the current user's local profile and only outside clone mode.
- The profile action row is labeled **Local showcase**. Clearing it writes only IntelGram's local setting.
- No creator/admin flags are synthesized. No ownership, permission, membership, username, or profile update request is added.

### Local Channel Workspace

- `Ayu::ShowLocalChannelWorkspace` resolves only the already-selected public username, then opens a dedicated **Local-only channel** box instead of mutating or injecting Telegram history.
- `localProfileChannelWorkspace` stores compact versioned JSON in IntelGram settings. It separates data by normalized channel username, keeps at most eight channel workspaces and 100 posts per workspace, and bounds text and local path lengths.
- The asynchronous public-channel resolver uses `base::make_weak` for `Window::SessionController` lifetime safety, matching Telegram Desktop's non-`QObject` controller pattern.
- Local posts contain an IntelGram-generated ID, local creation/edit timestamps, text, an optional local attachment path, and local pin state. Post actions edit, delete, pin, unpin, copy, or open that local attachment.
- Local channel settings override the workspace title, description, and photo path. They do not write into `ChannelData`, Telegram history, the media cache, or any MTProto channel method.
- The profile showcase row opens the local workspace and reacts to local title changes. The normal personal-channel path continues to open Telegram peer information.

### Validation

- `validate_intelgram_patch.py` requires the adaptive material controls, new optical render helpers, Liquid Bounce motion hook, local channel showcase UI and read-only lookup, and exact `lib_ui` commit.
- Mutation scans continue to reject profile updates, username availability checks, contact import, channel joins, personal-channel updates, gift transfers, and protected-content bypasses.
- Clean-source patch application, byte-identical alias, SHA-256, whitespace checks, localization-key uniqueness, exact submodule revision, platform compilation, launch smoke tests, and release-asset digest validation remain required before publication.
- Replacement platform compilation, launch, and packaging passed in macOS run `30192805382`, Windows run `30192805379`, and Linux run `30192805452`. Publisher run `30196372286` uploaded all 19 permanent assets, and public-download audit run `30196394410` passed every digest, checksum, report, asset-presence, latest-release, and patch-hash check.

## IntelGram v6.7.8 Glass And Motion - 2026-07-25

### Source Baseline And Patch

- Upstream source: official AyuGram Desktop `v6.7.8`, commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with required submodules.
- Source commit: `1ce3fa9ea` on the recovered local implementation branch.
- Delivery patch: [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch).
- Compatibility alias: [`ayugram-local-profile-render-overrides.patch`](ayugram-local-profile-render-overrides.patch), byte-for-byte identical.
- Patch SHA-256: `a749b1673bc93f057f0da292680f2884fac956d64229554272b0bb2666995599`.
- Patch footprint: 74 files, 8,436 insertions, and 488 deletions relative to the pinned source.
- `Telegram/lib_ui` now points to public fork [`foolspec/lib_ui`](https://github.com/foolspec/lib_ui), pinned at `2324a08c969967f62148b013eda0ae12507753ab`.

### Settings And Persistence

- Added `Settings::AyuVisualEffects` as a dedicated **Glass & Motion** page under the existing Customize group.
- `AyuSettings` persists the transparent-mode master switch; native blur; sidebar, chat, and panel surface switches; glass opacity and tint; local background path, opacity, and blur; enhanced-animation switch; motion preset; and window-opening switch.
- Numeric and color values are normalized during assignment and JSON loading. Missing keys retain conservative defaults, keeping both feature masters disabled after upgrade.
- The local background picker uses Telegram Desktop's image reader and stores only the selected local path. Clearing the setting releases the decoded image and returns to native blur or the selected theme.

### Window Glass Rendering

- `Ayu::VisualEffects` snapshots the current opaque palette, derives alpha-adjusted glass roles, and reapplies them when a theme changes.
- Sidebar, chat, and panel role groups are independent, while common input, hover, title, and window roles follow the master opacity.
- Disabling transparent mode restores the untouched palette snapshot instead of constructing a replacement theme.
- `MainWindow` uses a translucent Qt root surface and paints a scaled-cover local backdrop, optional `Images::BlurLargeImage` result, and glass tint behind normal child widgets.
- Text, icons, avatars, videos, and message media are not faded because opacity is applied to background palette roles and backdrop painting rather than to the complete window.
- IntelGram temporarily uses its custom frame while transparent mode is active so per-pixel transparency reaches the platform surface, then restores the user's normal frame preference when disabled.

### Native Backdrop Backends

- macOS inserts one associated `NSVisualEffectView` as a sibling ordered behind Qt's native content view, using behind-window blending and the under-window-background material where available. This avoids compositing the native material over Qt's text, controls, avatars, and media.
- Windows requests the DWM system backdrop and falls back to `DwmEnableBlurBehindWindow` on systems without the newer attribute.
- Linux leaves the Qt ARGB window transparent for the running compositor and does not assume a desktop-specific blur protocol.
- Every platform can render the same local image backdrop independently of the native blur path.

### Shared Motion Engine

- The pinned `lib_ui` fork adds process-wide `anim::MotionStyle`, duration scaling, and transition selection.
- `Ui::Animations::Simple::start` and `change` route existing animations through `MotionDuration` and `MotionTransition`, covering established IntelGram transitions without duplicating animation code at individual call sites.
- Quick Snap shortens motion and uses a fast quintic exit; Smooth Flow slightly lengthens cubic easing; Diabolical uses a brisk circular exit; Springy uses a monotonic critically damped curve.
- `Ayu::VisualEffects::ApplyMotionSettings` maps the persisted IntelGram preset into the shared UI engine and returns it to `Default` whenever Enhanced Animations is off.
- The optional main-window fade runs only when enhanced motion is enabled and the upstream `anim::Disabled()` accessibility/power-saving gate permits animation.

### Build And Validation

- All platform jobs resynchronize `Telegram/lib_ui` after patch application, force-check out the pinned public fork commit, and verify its exact SHA before branding or compilation.
- `validate_intelgram_patch.py` now requires the Glass & Motion settings, palette compositor, native platform backends, shared motion hooks, and exact `lib_ui` commit in addition to every previous mutation and protected-content boundary.
- Local validation includes patch alias equality, patch SHA-256, `git diff --check`, localization-key uniqueness, clean application to the pinned source, exact submodule checkout, validator execution, and reverse-application.
- Full compilation, packaging, isolated launch smoke tests, and public release-asset verification remain delegated to GitHub Actions under the upstream no-local-full-build instruction.
- Release `intelgram-v6.7.8-glass-motion-20260725` combines successful macOS run `30178929833`, Windows run `30178932569`, and Linux run `30178931089`; every package records patch `a749b1673bc93f057f0da292680f2884fac956d64229554272b0bb2666995599` and a passed launch smoke test. Publisher run `30181760408` uploaded all 19 permanent assets, and public-asset validation run `30181808944` passed every digest, checksum, report, asset-presence, latest-release, and patch-hash check.

## IntelGram v6.7.8 Appearance And Settings Refresh - 2026-07-24

### Source Baseline And Patch

- Upstream source: official AyuGram Desktop `v6.7.8`, commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with required submodules.
- Source commit: `eeba6a1ec` on the recovered local implementation branch.
- Delivery patch: [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch).
- Compatibility alias: [`ayugram-local-profile-render-overrides.patch`](ayugram-local-profile-render-overrides.patch), byte-for-byte identical.
- Patch SHA-256: `7e08df09395bd170067a0495019f9dcef66e172dc6492bbba27c4cf1d70cbef6`.
- Patch footprint: 62 files, 7,357 insertions, and 484 deletions relative to the pinned source.

### Native Appearance And Background Controls

- `Settings::AyuAppearance::setupContent` reuses Telegram Desktop's `Settings::SetupThemeOptions` for visual light/dark theme cards and `Settings::SetupCloudThemes` for the native in-app cloud-theme browser.
- `Settings::SetupChatBackground` supplies the live wallpaper preview, Telegram wallpaper gallery, local-image picker, tiling control, and wide-layout behavior.
- Telegram's native theme/palette/image import row and IntelGram's fourteen platform-native app-icon choices remain on the same focused page.
- A reactive **Remove current custom theme** row appears only for a non-embedded theme, confirms the action, closes an active editor safely, restores an embedded default, and delegates cloud-theme uninstallation to Telegram Desktop's existing theme manager.
- Removed the three bundled Windows 93, Terminal, and AMOLED novelty resources and their QRC registrations; appearance now follows Telegram Desktop's maintained visual and cloud-theme paths.
- Renamed the inherited custom-background switch so its label accurately describes ignoring backgrounds bundled with themes.

### Settings Information Architecture

- `Settings::AyuSettings::setupContent` groups the preferences home page into **Customize**, **Power tools**, and **Client settings**.
- `Settings::AyuOther` now owns only **Local Profile & Collectibles** controls.
- Added `Settings::AyuNavigation` for drawer, tray, and chat-folder controls.
- `Settings::AyuVault` now owns only search, timeline, moments, and smart-folder tools.
- Added `Settings::AyuAutomation` for contact notes, public identity snapshots, rules, anti-spam, and per-chat privacy controls.
- Added `Settings::AyuExport` for selected-message, chat, account, ZIP, and encrypted archive exports.
- Added `Settings::AyuAdvanced` for diagnostics, URL registration, and local reset controls.
- Corrected the settings version label to `IntelGram Desktop`.

### Validation

- The patch validator requires the native theme-card, cloud-theme, and chat-background hooks plus every new focused settings class.
- Validation fails if any retired novelty-theme resource is reintroduced.
- Localization-key uniqueness, QRC XML parsing, patch whitespace checks, byte-identical patch aliases, clean pinned-source application, and mutation-boundary scans remain required before packaging.
- Platform compilation, packaging, isolated launch tests, and public release-asset verification remain delegated to GitHub Actions in accordance with the upstream no-local-full-build instruction.
- Release `intelgram-v6.7.8-appearance-refresh-20260724` combines successful macOS run `30125712722`, Windows run `30125939832`, and Linux run `30125506279`; every package records patch `7e08df09395bd170067a0495019f9dcef66e172dc6492bbba27c4cf1d70cbef6` and a passed isolated launch smoke test. Publisher run `30139305324` uploaded all 19 permanent assets, and public-asset validation run `30139540010` passed every digest, checksum, report, asset-presence, latest-release, and patch-hash check.

## IntelGram v6.7.8 Vault Suite - 2026-07-23

### Source Baseline And Patch

- Upstream source: official AyuGram Desktop `v6.7.8`, commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with required submodules.
- Source commit: `2abc8dc30` on the recovered local implementation branch.
- Delivery patch: [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch).
- Compatibility alias: [`ayugram-local-profile-render-overrides.patch`](ayugram-local-profile-render-overrides.patch), byte-for-byte identical.
- Patch SHA-256: `566f49ca3979c62366bb8ebb4543d3cc7694037aa5781eaa4db031ec1b78b94d`.
- Patch footprint: 63 files, 7,106 insertions, and 461 deletions relative to the pinned source.

### Vault Storage And Indexing

- `IntelGram::Vault` owns a WAL-enabled, `secure_delete` SQLite database under IntelGram's work directory.
- `Data::Session::addNewMessage` indexes regular received/loaded messages; edited-message processing records the prior body before applying an edition and refreshes the current index row afterward.
- FTS5 indexes body text, media filename, and links with a `LIKE` fallback when FTS5 is unavailable.
- Account ID, signed dialog ID, message ID, sender, topic, date, media metadata, tags, unread state, and local deletion state form the structured message record.
- Search, smart folders, media history, timeline statistics, moments, profile notes, note history, identity snapshots, rules, rule matches, chat policies, and options are accessed through one mutex-guarded API.

### Protected-Content Enforcement

- `IsProtected` checks `HistoryItem::forbidsSaving`, peer forwarding permission, TTL destruction, unsupported TTL, and media TTL.
- Protected records persist empty body, links, media name, MIME type, and media path fields; only reference metadata remains.
- Legacy Ayu edit/deletion storage rejects protected and self-destructing items.
- Telegram's `Flag::NoForwards`, extended-media save restriction, media overlay checks, and TTL predicate are restored.
- `AyuForward::isAyuForwardNeeded` and full-forward variants return false, disabling inherited copy/re-send forwarding paths.
- Rule forward queues reject protected rows and re-check `HistoryItem::allowsForward()` before opening Telegram's native confirmation picker.
- Selected-message, chat, account, ZIP, and encrypted exports all reuse the same protected-row representation and exclude protected cached media.

### Timeline, Notes, Rules, And Chat Policy

- `Settings::AyuVault` adds native sections for local search, unified account views, timelines, smart folders, contact context, rules, anti-spam review, per-chat policy, themes, and exports.
- Saved moments use message references plus private title, note, and tags.
- Profile notes keep current state and append-only local history with reminder timestamps.
- Identity snapshots are explicit and record only public fields already present in `PeerData`.
- Rules match keyword, link, photo, or file metadata and can tag, save, alert, mute locally, queue manual forwarding, or mark spam.
- Chat policies supply tags, priority, download mode, read reminder, local-only draft preference, and local notification mute.
- Draft upload hooks in `ApiWrap` drop cloud-save requests only for chats with the local-only preference.
- Notification hooks suppress native and default notifications only when the local chat policy requests it.
- Auto-download hooks use Qt 6.2-compatible `QNetworkInterface` types for manual, Wi-Fi/Ethernet, and always modes.

### Export And Encryption

- `IntelGram::Export` generates JSON, Markdown, HTML, PDF, and ZIP output for an account, chat, or explicit message ID selection.
- The message context menu indexes the loaded selection, opens Telegram Desktop's native folder picker, and scopes records, revisions, moments, and cached media to the selection.
- Account JSON includes locally visible contacts, notes and history, public identity snapshots, rules and activity, chat policy, options, moments, revisions, and message records.
- JSON reports only whether permitted media is cached; host filesystem paths are not serialized.
- ZIP packaging streams permitted local files in chunks under sanitized archive names.
- Frozen Account Backup encrypts the ZIP stream using AES-256-GCM, a random 16-byte salt, a random 12-byte nonce, and PBKDF2-HMAC-SHA256 with 250,000 iterations.
- Encrypted mode writes only `.intelvault` output and does not leave the normal plaintext export set in the destination.
- Telegram authorization keys and session credentials are deliberately excluded.

### Themes And Resources

- Three validated `.tdesktop-theme` ZIP resources provide Windows 93, Terminal, and AMOLED palettes/backgrounds.
- Classic Telegram maps to the existing bundled day-blue theme.
- Theme Studio applies bundled resources or imports a user-selected Telegram theme without modifying account data.

### Validation

- `validate_intelgram_patch.py` checks required feature hooks, resources, Qt 6.2 compatibility, no-forward enforcement, protected media checks, and added-line mutation references on every platform.
- `git diff --check`, localization-key uniqueness, QRC XML parsing, and all three theme ZIP integrity checks pass locally.
- Both public patch names have the same SHA-256 and produce a byte-identical diff after clean application to the pinned official source.
- The Linux compiler pass removed a stale `base/functional.h` include that is not present in the pinned source tree; `not_null` remains supplied by Telegram Desktop's standard precompiled header.
- The full Linux diagnostic compile corrected the vault passphrase wrapper, pinned-revision menu icon names, and plural-count producer in `settings_vault.cpp`.
- Cold-cache macOS dependency preparation is split into six bounded, cumulative cache stages before the independent application compile and launch-test job.
- macOS and Windows dependency cache creation is serialized to stay within GitHub's repository cache quota, installed Qt source trees are removed after a verified install, and macOS can resume from a completed Qt-stage cache.
- Every Windows dependency caller uses an explicit success-gated `always()` condition so a Windows-only run continues after the intentionally skipped macOS job instead of reporting a misleading success after stage one.
- Completed macOS and Windows dependency-run IDs can be reused explicitly while their exact caches remain available; missing caches fail closed instead of silently building against a partial dependency tree.
- The upstream `AGENTS.md` instruction to avoid a local full build is preserved; macOS, Windows, and Linux compiles and isolated launch tests run in GitHub Actions.

## IntelGram v6.7.8 Local Profile Update - 2026-07-19

### Source Baseline And Patch

- Upstream source: official AyuGram Desktop `v6.7.8`, commit `b25513a06ff88be0b3f4c928252b56c3da39cec7`, with required submodules.
- Delivery patch: [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch).
- Compatibility alias: [`ayugram-local-profile-render-overrides.patch`](ayugram-local-profile-render-overrides.patch), byte-for-byte identical.
- Patch SHA-256: `ae6e8dbdfc3c9daee6c565800e8ef55c840a8b29172d6dd0d5d55790b5415de7`.
- Patch footprint: 38 source files, 3,185 insertions, and 433 deletions relative to the local baseline snapshot.

### Native Collectible Galleries

- `ShowLocalGiftCollectionPicker` uses `Ui::MakeGiftsList` as the primary collection surface instead of generating text-only settings rows.
- `LoadLocalGiftCollections` requests Telegram's read-only star-gift catalog and maps each resell title to a clickable native collection tile.
- `LoadLocalGiftCollectionFallback` resolves one read-only sample from each established collection when the catalog does not provide usable collection tiles.
- `ShowLocalGiftCollectionBrowser` resolves exact numbered slugs in pages of 12, appends them to a scrollable native gift grid, and stores the clicked gift reference locally.
- Exact gift resolution remains `MTPpayments_GetUniqueStarGift`; raw TON NFT addresses may use a read-only TonAPI metadata GET before resolving the Telegram slug.

### Settings And Preview Organization

- `BuildLocalProfile` now separates clone controls, identity, usernames/bio/contact, profile photo, and collectibles with native subsection headings and dividers.
- Removed the duplicate collectible-browser action; featured and pinned flows both enter the same native collection gallery.
- `SetupPeerColorSample` now consumes `Ayu::LocalProfileNameValue(peer)`, so the name-color preview reacts to the local display-name setting and clone state.

### Local Gift Detail Presentation

- `Core::ResolveAndShowUniqueGiftForLocalProfile` reuses the existing read-only unique-gift resolver and Telegram detail box for locally featured, pinned, and cloned collectibles.
- The resolver deep-copies the fetched `Data::UniqueGift` presentation object and substitutes only `originalDetails.recipientId` with the signed-in peer for the local detail view.
- `StarGiftResaleInfo::localProfileRecipientId` overrides only the detail entry's displayed Telegram host peer, so the native Telegram profile chip uses the same signed-in peer while the unique gift's actual `hostId` and `ownerId` remain intact.
- The recipient label and peer-table value consume `Ayu::LocalProfileName`, so they show the real display name when no local name is enabled and the local or cloned display name when one is active.
- `TopBar` compares configured local gift slugs case-insensitively before asynchronous collectible IDs resolve, routing featured and pinned local gifts through the local-profile detail resolver immediately.
- Telegram's existing click handlers therefore open the signed-in user's short profile from either surface. `PrepareShortInfoBox` consumes `LocalProfileName`, `LocalProfilePhone`, `LocalProfileUsername`, `LocalProfileAbout`, and `LocalProfilePersonalChannel`; when a local or cloned userpic is active, `ProcessCurrent` bypasses the real Telegram photo-history item and renders the central override as the card's single large, fully loaded photo instead of a blurred server-photo placeholder.
- Locally overridden usernames are shown without linking to an unrelated public server username; normal non-local username links remain unchanged.
- `ownerId`, sender, date, price, transfer, resale, and every underlying Telegram or on-chain ownership field remain unchanged. Normal non-local gift details continue through the original resolver.

### Profile Clone Fidelity And Username Editor

- `BadgeValue` dynamically switches its render source to `Ayu::LocalProfileCloneUser`, mirroring premium, verified, scam, and fake badge state without changing `UserData` or session entitlements.
- `BotVerifyBadgeForPeer` uses the same reactive clone source so organization verification symbols follow the cloned profile.
- `Ayu::LocalProfilePersonalChannelValue`, `LocalProfilePersonalChannel`, and `LocalProfilePersonalChannelMessageId` drive the own-profile personal-channel block from the clone, including the channel message preview.
- A clone with no personal channel produces `nullptr`, which hides the block even when the real self profile has a channel; cloned-channel edit context actions are suppressed.
- `ShowLocalProfileCloneBox` calls the existing `requestFullPeer` read path after selecting an already-loaded user so visible full-profile badge and personal-channel fields are refreshed without changing either account.
- `ShowLocalProfileUsernameEditor` now uses Telegram's `lng_username_title`, native `Ui::UsernameInput`, `@username` label, `st::usernamePadding`, `AddUsernameCheckLabel`, native two-part help copy, and standard Save/Cancel layout.
- `LocalUsernameEditor` wraps the native non-reactive `Ui::UsernameInput` in `Ui::RpWidget`, matching Telegram's own editor geometry while satisfying `Ui::GenericBox::addRow` on Clang and GCC.
- The wrapper adapts `Ui::MaskedInputField::changed` and `submitted` into local reactive streams used by the validation label and Save action.
- The editor no longer references the custom `Local username` title or IntelGram-only disclaimer. Its syntax result and save are computed locally and never call `MTPaccount_CheckUsername`, `MTPaccount_UpdateUsername`, or another account mutation method.
- Removed the added `Original Telegram username` strip and its localization keys.

### Collectible Username And Number Presentation

- `SessionNavigation::resolveCollectible` keeps Telegram's existing read-only collectible lookup and passes the resolved owner peer into the native information box.
- The collectible parser now supplies `Ayu::LocalProfileName(owner)` to the owner chip, pairing the already locally rendered avatar with the active local or cloned display name. Non-self peers continue to fall back to their normal Telegram name.

### Cross-Platform Icon Assets

- `build_intelgram_branding.py` installs a pink primary icon, a pink profile-art alternate, and twelve color variants into the existing runtime icon picker.
- The icon generator produces 1024px masters, seven-size Windows `.ico` files, macOS `.icns` and `.icon` resources, and Linux hicolor assets from 16px through 1024px.
- macOS icon-composer packages now use dedicated opaque full-bleed PNGs at `1.0` scale. Character sources keep their native platform background and color variants flatten transparent corners to the icon's sampled background, preventing Icon Composer from adding a visible white shell behind an already-rounded image.
- The primary application icon, installer resources, alternate macOS icon list, Qt resources, and picker constants are generated together from `branding/icons`.
- Stored use of the retired duplicate `chibi2` picker slot migrates to the new primary icon.

### Settings And Project Links

- Removed `BuildDonations` and its Boosty, TON, Bitcoin, Ethereum, Solana, and Tron rows from **Settings -> Other**.
- Added first-party rows for `@intelgrams`, the bundled update log, `foolspec/IntelGram`, `CHANGELOG.md`, and `TECHNICAL_CHANGELOG.md`.
- The Telegram row calls `Window::SessionController::showPeerByLink` and leaves joining to Telegram's normal channel UI.
- External documentation rows use `QDesktopServices::openUrl` with explicit GitHub URLs.

### In-App Update Log

- `ShowIntelGramUpdateLog` opens an existing `Ui::GenericBox` from the IntelGram settings page.
- The bundled localized content is split into latest-update, main-feature, privacy-boundary, and credit sections using `Ui::AddSubsectionTitle` and `Ui::AddDividerText`.
- The dialog requires no network access to display its update summary.
- An explicit **View full changelog** action opens `CHANGELOG.md` on GitHub for the complete history.
- Included `lang/lang_text_entity.h` directly so the update log's `tr::rich` formatter compiles on every supported platform.
- The multiplatform workflow checks for `ShowIntelGramUpdateLog` after applying the source patch.

### Consent-Based Supporter Badge

- `isIntelGramChannelMember` checks the already-loaded `@intelgrams` peer through `Data::Session::peerByUsername` and `ChannelData::amIn`.
- `isSupporterPeer(not_null<PeerData*>)` combines existing upstream supporter data with the local IntelGram membership-derived state for the user's own peer.
- `ExteraBadgeTypeFromPeer` merges its initial badge value with `Data::PeerUpdate::ChannelAmIn` and `Username` updates, allowing the badge to react after a normal join or leave.
- Existing badge renderers in the settings cover, main menu, profile top bar, and unread badge path now use the peer-aware supporter check.
- Clicking the IntelGram supporter badge opens `@intelgrams`; it does not call a join method.
- No `MTPchannels_JoinChannel`, automatic join, background subscription, or other channel mutation was added.

### Local Profile Architecture

- `AyuSettings` persists enable flags and normalized local values for display name, UID, usernames, anonymous number, bio, photo path, clone UID, featured gift, and pinned gifts.
- `Ayu::LocalProfileNameValue`, `LocalProfileIdValue`, `LocalProfileUsernameValue`, `LocalProfileUsernamesValue`, `LocalProfilePhoneValue`, and `LocalProfileAboutValue` provide the reactive render values.
- `Ayu::LocalProfilePhotoImage`, `LocalProfileUserpicActive`, `LocalProfileEmojiStatusId`, `LocalProfilePersonalChannelValue`, and `LocalProfileGiftReferencesValue` supply local photo, clone, badge-adjacent, personal-channel, emoji-status, and collectible presentation data.
- `Ayu::RefreshLocalProfilePresentation` emits existing peer update flags so already-open UI surfaces redraw without a Telegram profile update.
- `PeerData` userpic paths and targeted dialog, history, settings, profile, table-row, and main-menu call sites consume the local values only when the rendered peer is the signed-in user.

### Search And Clone Boundaries

- UID and phone search use `Data::Session::userLoaded` and `userByPhone`, limiting results to peers already present in the local session cache.
- Recognized UID/phone input bypasses normal remote username and message search for that query.
- Profile cloning accepts only a loaded non-self user, refreshes that user's normally visible full profile read-only, and locally mirrors visible fields, badges, emoji status, and personal channel; it does not fetch hidden data or alter either account or channel.

### Validation

- Manual workflow runs can target `linux`, `macos`, `windows`, or `all`; platform-scoped concurrency lets compile checks run without cancelling unrelated dependency preparation.
- A Windows-only run can reuse the exact final dependency cache from a completed run ID and attempt, skipping all nine preparation jobs while preserving the final-cache marker check.
- `git diff --check` passes.
- Both patch filenames have identical SHA-256 digests.
- Clean patch application and reverse-application checks pass against the pinned baseline snapshot.
- Added-line scans find no channel-join, profile-update, contact-import, gift-transfer, sale, purchase, or ownership mutation request.
- The only custom network paths are the standard read-only full-peer refresh after clone selection, read-only collectible catalog/detail resolution, and optional read-only TON NFT metadata resolution; local recipient presentation reuses the same detail response without sending any write request.
- A full local build was intentionally not run because the upstream repository's `AGENTS.md` says to avoid building the project.

### Release State

- Release validation compares the current source and release patch hashes, downloads every package, runs each published checksum file, and checks GitHub's package digests against those checksums.
- Temporary validation copies normalize platform-native checksum and report line endings before GNU checksum verification.
- Every platform validation report must record the current patch hash and a passed launch smoke test; all launch logs and packaging inputs must also be present.
- The manual release publisher accepts separate successful macOS, Windows, and Linux run IDs, verifies each required package and validation report, and combines them into one permanent release.
- Release notes enumerate the main local-only features, supported packages, explicit supporter-join behavior, privacy boundary, credit, and links to both changelogs.
- A launch test that produces no console output is retained as an explicit silent-launch success log instead of being omitted from release assets.
- Release `intelgram-v6.7.8-local-profile-20260720` combines successful macOS run `29701604530`, Windows run `29701679512`, and Linux run `29701680681`; each package records patch `56e12dad016d54f7c7f917409fba34c4ca935ba746b261ac8383ed710b9762e9` and a passed launch smoke test before publication. Publisher run `29775866172` assembled the permanent release, and public-asset validation run `29775945944` passed every digest, checksum, report, and patch-hash check.
- Release `intelgram-v6.7.8-local-profile-20260720-2` combines successful macOS run `29777684364`, Windows run `29777689027`, and Linux run `29777686484`; each package records patch `ae6e8dbdfc3c9daee6c565800e8ef55c840a8b29172d6dd0d5d55790b5415de7` and a passed isolated launch smoke test. Publisher run `29789518828` assembled the replacement release, and public-asset validation run `29789551493` passed every digest, checksum, report, and patch-hash check.
- Release `intelgram-v6.7.8-vault-suite-20260723` combines successful macOS run `30043488045`, Windows run `30054931480`, and Linux run `30042195560`; every package records patch `566f49ca3979c62366bb8ebb4543d3cc7694037aa5781eaa4db031ec1b78b94d` and a passed launch smoke test. Publisher run `30115073189` uploaded all 19 permanent assets, and public-asset validation run `30117089125` passed every digest, checksum, report, asset-presence, and patch-hash check.

## Initial IntelGram Local Profile Implementation

### Rendering

- Added centralized own-user render helpers and reactive values instead of changing Telegram's stored `UserData` profile fields.
- Routed local presentation through the settings cover, main menu, dialog rows, search results, chat and profile headers, messages, replies, forwarded previews, about rows, table rows, and avatar paths.
- Preserved normal Telegram rendering for every other peer.

### Local Fields And Collectibles

- Added local display name, UID, primary and secondary usernames, anonymous number, bio, profile photo, cached-profile cloning, featured collectible, and up to six pinned collectibles.
- Added the original Telegram username reference to the local username editor.
- Used Telegram's native unique-gift model, artwork, tooltip, and detail surfaces for resolved collectibles.

### Packaging

- Added deterministic product-facing IntelGram branding and separate macOS, Windows, and Linux application identifiers.
- Added GitHub workflows for clean-source patch verification, platform packaging, launch smoke tests, checksums, release publication, and release validation.
