# Changelog

All notable IntelGram custom-feature changes are recorded here.

IntelGram uses sequential product versions beginning with v1. The retained `v6.7.8` references identify the pinned upstream AyuGram Desktop source or legacy build tags, not the IntelGram release number.

## Release History

| Version | Published | Update |
| --- | --- | --- |
| v19 | 2026-08-03 | Exact Telegram Disguise Identity |
| v18 | 2026-08-02 | macOS Stability And Telegram Icon |
| v17 | 2026-07-31 | Reliable Telegram Disguise Trigger |
| v16 | 2026-07-31 | Telegram Disguise Mode |
| v15 | 2026-07-29 | Local Owner Controls And Profile Badges |
| v14 | 2026-07-27 | Native Local Ownership |
| v13 | 2026-07-26 | Local Channel Workspace |
| v12 | 2026-07-26 | Liquid Glass |
| v11 | 2026-07-26 | Glass & Motion |
| v10 | 2026-07-25 | Appearance Refresh |
| v9 | 2026-07-24 | Vault Suite |
| v8 | 2026-07-21 | Owner Name Rendering |
| v7 | 2026-07-20 | Profile Card Rendering |
| v6 | 2026-07-19 | Collectible Gallery And Clone Fidelity |
| v5 | 2026-07-19 | Update Log And Supporter Badge |
| v4 | 2026-07-18 | Profile Cloning And Collectible Browser |
| v3 | 2026-07-14 | Multiplatform Launch |
| v2 | 2026-07-13 | Local Profile Build Refresh |
| v1 | 2026-07-13 | Initial Local Profile |

## IntelGram v19 Exact Telegram Disguise Identity - 2026-08-03

### Fixed

- Changed the hidden-mode application identity from the generic Telegram Desktop label to the exact running title **Telegram**.
- Updated the live macOS Launch Services identity so the Dock label changes to **Telegram** immediately and returns to **IntelGram** when hidden mode is undone.
- Replaced the macOS alternate-icon catalog lookup with direct rendering of the selected app image, preventing the Telegram icon from appearing as a tiny tile inside a white frame.
- Replaced the circular runtime Telegram image with the same rounded-square artwork used by the normal Telegram macOS application.

### Restore And Privacy

- To undo hidden mode, open the main drawer and click the **Telegram Desktop** footer three times within 1.2 seconds.
- The change affects only this client's local process name, menus, title, and icon. It does not alter a Telegram account, profile, channel, message, or server-side setting.

## IntelGram v18 macOS Stability And Telegram Icon - 2026-08-02

### Fixed

- Prevented macOS global-menu refreshes from dereferencing Ghost Mode actions that are intentionally absent while Telegram disguise mode is active. This fixes the focus-change crash reached when opening `@username` links.
- Made the own-profile username QR visibility subscription retain its peer directly instead of reading a destroyed temporary profile-layout helper during later username updates.
- Made Settings search calculate Vault reminder counts from its always-present session instead of dereferencing a window controller that does not exist during search indexing.
- Replaced the disguised macOS circular source artwork with Telegram's full rounded-square icon so the Dock no longer shows a tiny logo inside a second white tile.
- Reduced Liquid Glass background work by pacing its slow decorative highlight layer at 30 Hz instead of repainting the whole window at up to 120 Hz; scrolling and normal UI animations remain independent.

### Validation

- Added release checks for the guarded macOS menu state, lifetime-safe username subscription, session-safe Settings search, and exact rounded-square Telegram icon asset.
- The fixes change only local UI lifetime handling and application artwork; no Telegram account, profile, username, channel, or message mutation was added.

## IntelGram v17 Reliable Telegram Disguise Trigger - 2026-07-31

### Fixed

- Replaced title-link activation counting with physical left-button event counting across the complete blue **IntelGram Desktop v...** heading.
- Applied the same physical-click handling to the **Telegram Desktop** drawer footer used to restore IntelGram.
- Expanded the three-click sequence window from 650 ms to 1.2 seconds so normal rapid clicks register reliably without making the hidden gesture easy to trigger accidentally.
- Prevented Qt's double-click text-selection behavior from swallowing the second click in the sequence.

### Local-Only Boundary

- The corrected gesture still changes only this installation's product presentation and stored IntelGram preference.
- It sends no Telegram account, profile, icon, session, or application-identity request.

## IntelGram v16 Telegram Disguise Mode - 2026-07-31

### Added

- A hidden three-click trigger on the large **IntelGram Desktop v...** heading in IntelGram Preferences.
- A live Telegram Desktop heading and Telegram logo preview when the mode activates.
- An in-app Update Log entry explaining how to enter and leave the mode.

### Changed

- Enabling disguise mode from Preferences now returns directly to Telegram's normal Settings screen so IntelGram-only labels are not left visible.
- The existing three-click **Telegram Desktop** drawer-footer gesture remains the restore path.
- The selected identity persists across restarts and refreshes the app name, window title, Dock or taskbar icon, tray icon, menus, About surface, and drawer branding.
- Local profile, collectible, badge, and channel presentation remains active while the IntelGram product identity is hidden.

### Local-Only Boundary

- Disguise mode changes only this installation's product presentation and stored IntelGram preference.
- It sends no Telegram account, profile, icon, session, or application-identity request.

## IntelGram v15 Local Owner Controls And Profile Badges - 2026-07-29

### Added

- Telegram's native reaction strip on synthetic local channel broadcasts.
- Owner-style Reply, Edit, Pin/Unpin, Copy Text, Copy Post Link, Forward, Delete, and Select rows in the local post context menu.
- Persistent local replies, edited text, pin state, arbitrary emoji reaction counts, custom-emoji reaction counts, and paid-Star display counts.
- A **Broadcast engagement** editor with configurable starting views, maximum views, increment size, interval, six baseline reactions, and paid Stars.
- On-device view growth that stops at the selected maximum.
- A compact **Profile badges** editor with Automatic, no-primary-badge, Premium, verified, SCAM, FAKE, and DIRECT choices.
- Optional local emoji-status and organization-verification document IDs rendered through Telegram's native custom-emoji badge system.

### Changed

- Restored pinned local posts now repopulate Telegram's local pinned-message index and pinned bar.
- Native reaction clicks and the favorite-reaction shortcut increment only the selected local counter.
- The local Forward row performs a safe text handoff; synthetic local message IDs never enter Telegram's forwarding API.
- The in-app update log and public feature guide now document the complete owner-style post menu.
- Automatic badge mode now mirrors every supported primary badge, emoji status, and organization-verification symbol from the selected clone profile and clears missing elements.

### Local-Only Boundary

- No local post, view, reaction, Stars payment, reply, pin, edit, delete, or synthetic-message forward is submitted to Telegram.
- Existing guards continue to block unsupported post types before upload or send paths.
- Badge choices are IntelGram render settings; no Telegram account, username, emoji-status, or verification request is added.

## IntelGram v14 Native Local Channel Ownership - 2026-07-27

### Added

- **Local ownership** in the IntelGram submenu of every broadcast channel.
- Telegram's native **Broadcast a message...** composer inside the selected channel.
- Persistent local text broadcasts rendered directly in the channel's real timeline.
- Owner-style **Manage channel** entries in channel menus and profile actions.
- Per-account, per-channel local storage with individual delete and clear-all actions.
- A local-only silent-broadcast toggle that never updates Telegram notification settings.

### Changed

- Join, Leave, and Report controls are hidden while local ownership is active.
- Saved local broadcasts are restored when the channel is reopened and removed from the live timeline when the mode is disabled.
- The in-app update log now separates native local ownership from the existing profile showcase workspace.

### Local-Only Boundary

- Text broadcasts are intercepted before Telegram's send API and use client-local message IDs.
- Media, files, voice messages, stickers, inline results, forwards, and scheduled posts are blocked before upload or send paths.
- No creator/admin flag, channel right, membership value, channel information, or Telegram message is changed.

## IntelGram v13 Liquid Glass And Local Channels - 2026-07-26

### Added

- A layered Liquid Glass material with adaptive tint and contrast, wallpaper-colored light diffusion, moving cursor-reactive highlights, subtle edge refraction, soft internal reflections, anti-banding grain, and anti-aliased edge light.
- Separate controls for adaptive material, dynamic highlights, refractive edges, and optical-material intensity.
- **Liquid Bounce**, an optional high-elasticity animation pack with controlled overshoot.
- Plain-language descriptions of Quick Snap, Smooth Flow, Diabolical, Springy, and Liquid Bounce inside **Glass & Motion**.
- A clearly labeled local channel showcase that can render a public channel on your own IntelGram profile after a read-only public username lookup.
- A **Local-only channel** workspace opened from the showcase or Local Profile settings.
- Local channel posts with text and optional local-file attachments, plus edit, delete, pin, unpin, copy, and open-attachment actions.
- Local channel settings for a private title, description, and photo, with reset and delete-all controls.

### Changed

- Local-image backdrops now cache their blurred and scaled-cover results, avoiding image resampling on every animated frame.
- Material animation follows the active display refresh rate up to 120 Hz and pauses while the window is hidden, inactive, or reduced motion is enabled.
- The whole-window renderer now reacts to backdrop luminance and theme colors instead of applying one fixed transparent tint.
- The local channel showcase explicitly states that it is not ownership and does not join a channel, grant permissions, or modify Telegram.
- Local channel workspaces persist separately for up to eight selected channels and retain up to 100 posts per channel without issuing Telegram channel-write requests.

### Motion Packs

- **Quick Snap:** compact and immediate.
- **Smooth Flow:** calm and critically damped.
- **Diabolical:** brisk with stronger depth.
- **Springy:** a gentle settle without overshoot.
- **Liquid Bounce:** the most elastic option, with a controlled overshoot.

## IntelGram v11 Glass & Motion - 2026-07-26

### Added

- A dedicated **Glass & Motion** settings page, separate from Telegram themes and chat wallpaper controls.
- Opt-in whole-window transparency with native backdrop blur on macOS and Windows and compositor transparency on Linux.
- Independent glass controls for the chat list/sidebar, chat and message surfaces, and menus/dialogs/panels.
- Glass tint and surface-opacity controls that leave text, icons, avatars, and media crisp.
- A removable local whole-app background image with independent opacity and blur.
- Enhanced Animations with Quick Snap, Smooth Flow, Diabolical, and Springy presets.
- An optional window-opening and restore-from-tray animation.

### Changed

- Existing IntelGram transitions, dialogs, drawers, controls, and navigation now share one selectable motion profile when Enhanced Animations is enabled.
- Transparent mode follows live theme changes and restores the untouched theme palette immediately when disabled.
- Reduced-motion and power-saving behavior remains authoritative over every added animation.
- The profile username editor now uses Telegram's native title, `@username` control, validation row, help copy, spacing, and buttons while retaining IntelGram's local-only save path.

### Platform Support

- macOS uses `NSVisualEffectView` behind the Qt content surface.
- Windows uses the DWM system backdrop with the legacy blur API as a fallback.
- Linux keeps the Qt ARGB surface transparent for the active desktop compositor.
- Every platform can use a local image backdrop when native desktop blur is unavailable or undesired.

## IntelGram v10 Appearance & Settings Refresh - 2026-07-25

### Added

- Telegram's native visual theme-card selector directly inside **Appearance & Backgrounds**.
- In-app cloud-theme browsing with previews and no external theme website.
- A complete chat-background editor with Telegram's wallpaper gallery, local-image selection, live preview, tiling, and wide-layout controls.
- A visible **Remove current custom theme** action for uninstalling the selected custom theme and returning safely to a built-in default.
- Separate **Navigation & Layout**, **Contacts & Automation**, **Export & Backup**, and **Advanced & Maintenance** settings pages.
- **Customize**, **Power tools**, and **Client settings** groups on the IntelGram Preferences home page.

### Changed

- Renamed **Other** to **Local Profile & Collectibles** and removed unrelated maintenance controls from that page.
- Renamed **Vault & Tools** to **Vault & Search** and limited it to search, timeline, moments, and smart folders.
- Moved contact notes, identity inspection, rules, privacy, and anti-spam controls into **Contacts & Automation**.
- Moved local export and Frozen Account Backup into **Export & Backup**.
- Moved chat-folder, tray, and drawer controls out of Appearance and into **Navigation & Layout**.
- Removed the bundled Windows 93, Terminal, and AMOLED novelty packs in favor of Telegram's native visual and cloud theme catalogs.
- Clarified that the background preference for imported themes controls only backgrounds bundled inside theme files.

## IntelGram v9 Vault Suite - 2026-07-24

### Added

- An on-device SQLite message vault with FTS5 search across messages, media metadata, links, and filenames already received by IntelGram.
- Current-account search, all-account search, and a unified inbox with account labels.
- Chat jump-to-date, conversation statistics, compact media history, locally observed edit/deletion history, and private saved moments.
- Smart folders for unread people, work, high priority, needs reply, and spam review.
- Private contact notes, tags, relationship context, note history, reminders, and opt-in public identity snapshots.
- A local rules engine for keyword, link, photo, and file triggers with tag, save, alert, local mute, spam-review, and confirmed ordinary-message forwarding actions.
- Per-chat download behavior, read reminders, local-only draft preference, and local notification muting.
- Theme Studio with Windows 93, Terminal, Classic Telegram, AMOLED, and custom theme import.
- HTML, PDF, Markdown, JSON, and ZIP export for a selected message set, current chat, or current account.
- AES-256-GCM Frozen Account Backup with streamed encryption and permitted cached-media packaging.
- A dedicated **Vault & Tools** settings section and an updated in-app IntelGram log.
- Cross-platform patch validation for the new feature hooks, Qt 6.2 compatibility, mutation boundaries, and protected-content handling.

### Changed

- The complete public patch now contains the original local-profile, profile-clone, UID/visible-phone search, collectible, profile-photo, badge, icon, and branding work plus the Vault Suite.
- Rule forwarding is a confirmation queue that delegates to Telegram's native forward picker only when `allowsForward()` succeeds.
- Wi-Fi-only media behavior uses the Qt 6.2-compatible active network-interface API.
- Selected-message export indexes the loaded selection immediately, then scopes structured output, revisions, moments, and cached media to those message IDs.

### Protected Content

- Restored Telegram's no-forward flag and extended-media save restriction in the inherited Ayu paths.
- Protected and self-destructing messages store only account/dialog/message/date metadata and a local jump-back reference.
- Protected bodies, links, filenames, cached paths, revisions, rule payloads, and media never enter an export or backup.
- IntelGram adds no restricted-content forwarding or saving bypass.

## IntelGram v8 Local Profile Update - 2026-07-21

### Added

- A native visual collection gallery that replaces the text-only collectible collection list.
- A scrollable in-app gallery of exact numbered collectibles; clicking artwork selects it locally without opening Getgems.
- Pink IntelGram primary and profile-art icons plus twelve coordinated color variants for macOS, Windows, and Linux.
- A native in-app update log with the latest changes, main feature summary, privacy boundary, project credit, and full-changelog action.
- IntelGram Telegram, GitHub, changelog, and technical changelog links inside the app.
- A local supporter badge derived from membership in `@intelgrams` after the user joins through Telegram's normal channel page.
- A detailed technical changelog covering source hooks, persistence, network boundaries, and validation.
- Live collection chooser populated from Telegram's complete read-only star-gift catalog.
- Native visual collection tiles with an eight-collection offline fallback.
- Scrollable exact-number collectible browser with incremental loading.
- Local cached-peer search by UID or visible phone number in the main chat-list search field.
- `Found by ID or phone` result heading and normal profile-row interaction.
- Clone fidelity for premium and verification badges, organization badge symbols, emoji status, and personal channels.
- Complete feature inventory, conversational feature guide, and build update log.
- Local gift-detail recipient presentation for featured, pinned, and cloned collectibles.

### Changed

- Reorganized the local-profile page into identity, contact, photo, and collectible sections.
- Restored the familiar inline username-status row and removed the added original-username strip from the local editor; the status remains local and performs no Telegram availability request.
- Cloned badge, status, and personal-channel elements now disappear locally when the source profile lacks them instead of falling back to the real self profile.
- Clone selection now refreshes the already-known source user's full profile read-only so visible badge and personal-channel metadata is ready for local rendering.
- The name-color preview now follows the active locally rendered display name instead of the real-profile short name.
- Locally presented gift details now name the signed-in user with the active real or local display name in both the recipient link and Telegram profile chip; both open the signed-in user's profile without rewriting ownership or transaction metadata.
- Local collectible clicks now recognize configured gift slugs before asynchronous collectible IDs resolve, so the Telegram row cannot fall back to the original owner's profile.
- The compact profile opened from a local collectible now consumes the local display name, large profile photo, phone, usernames, bio, and personal channel.
- Collectible username and anonymous-number information boxes now pair the locally rendered avatar with the active local or cloned display name instead of the underlying Telegram peer label.
- Rebuilt every Windows icon as a seven-size `.ico` and added matching native macOS and Linux assets.
- Rebuilt macOS icon-composer sources as full-bleed platform artwork at 100% scale, removing the extra white shell and double inset around the Dock icon.
- Removed the inherited Boosty and cryptocurrency donation block from **Settings -> Other**.
- Collection thumbnails now come from Telegram's read-only catalog, with read-only sample resolution as a fallback for established collections.
- UID and phone queries now stay inside the local peer cache instead of entering normal remote message, topic, or username search.
- Main README now leads with IntelGram's custom features and links to the full documentation.
- Product-facing main-window, settings, version, and notification-preview titles now consistently use IntelGram.
- Completed the visible branding pass across the login footer, platform menus, About and crash dialogs, tray labels, updater identity, and Windows shortcut and installer metadata.
- The final Windows Qt dependency stage now runs serially to avoid a generated-directory race in `qtimageformats`.

### Privacy

- No automatic channel join was added; `@intelgrams` membership requires the user's normal Telegram **Join** action.
- No Telegram profile/account mutation was added.
- No contact import or address-book lookup was added.
- Collectible catalog, item, and TON metadata resolution remains read-only.

## IntelGram v1 Initial Local Profile Release - 2026-07-13

### Added

- IntelGram product and package identity for macOS, Windows, and Linux.
- Local name, UID, primary username, other usernames, anonymous number, bio, and profile photo.
- Original Telegram username reference in the local username editor.
- Local profile cloning by the UID of an already-loaded user.
- Local featured and pinned collectible rendering with native gift details.
- Central own-user avatar override and broad name/profile render hooks.
- Automated patch validation, mutation scanning, launch smoke testing, checksums, and release packaging.
