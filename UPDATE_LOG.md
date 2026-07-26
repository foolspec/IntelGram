# IntelGram Update Log

## Current Update

- Base source: official AyuGram Desktop `v6.7.8` at `b25513a06ff88be0b3f4c928252b56c3da39cec7` with required submodules.
- Product identity: IntelGram on macOS, Windows, and Linux with separate application identifiers.
- Source delivery: one complete IntelGram patch plus deterministic branding and validation scripts.
- Build delivery: GitHub Actions packages macOS Apple Silicon, Windows x64, and Linux x64.
- Build reliability: macOS and Windows dependency preparation uses bounded cache stages, platform cache creation is serialized, installed Qt source trees are pruned, macOS can resume directly from a completed Qt stage, and Windows-only runs carry every dependency stage through to the packaged application.
- Universal vault: local FTS search covers received messages, media metadata, links, and filenames, with current-account, all-account, and unified-inbox views.
- Timeline tools: jump-to-date, conversation statistics, compact media history, saved moments, and locally observed edit/deletion history.
- Organization: smart folders, private contact notes and reminders, note history, opt-in public identity snapshots, and account-scoped local rules.
- Chat controls: local tags, priority, manual/Wi-Fi/always download behavior, read reminders, local-only draft preference, and local notification muting.
- Anti-spam: unknown-sender and suspicious-invite review tags plus keyword, link, photo, and file rules.
- Appearance & Backgrounds: native visual theme cards, in-app cloud themes, imported theme files, a visible remove-current-theme action, Telegram's wallpaper gallery, custom local images, live background preview, tiling controls, and the IntelGram icon set.
- Liquid Glass: cached native or local-image blur with adaptive tint and contrast, wallpaper-colored diffusion, cursor-reactive highlights, moving specular reflections, subtle edge refraction, internal reflections, anti-banding grain, and anti-aliased edges.
- Optical controls: separate switches for adaptive material, dynamic highlights, refractive edges, and an intensity slider, alongside the existing sidebar/chat/panel, tint, opacity, and removable backdrop controls.
- Material performance: active highlights follow the display refresh rate up to 120 Hz while blurred and scaled image results are cached; repainting pauses while hidden, inactive, or reduced motion is enabled.
- Enhanced Animations: Quick Snap, Smooth Flow, Diabolical, Springy, and Liquid Bounce apply across existing transitions, dialogs, drawers, controls, and navigation, with an optional window-opening animation.
- Motion guide: Quick Snap is immediate, Smooth Flow is critically damped, Diabolical is brisk with stronger depth, Springy settles gently, and Liquid Bounce adds the strongest controlled overshoot.
- Accessibility: IntelGram's reduced-motion and power-saving switch remains authoritative and disables the added motion behavior.
- Export Center: selected-message, chat, and account HTML/PDF/Markdown/JSON/ZIP export.
- Frozen Account Backup: streamed AES-256-GCM `.intelvault` output with permitted cached media and no plaintext destination files.
- Existing feature scope remains: local profile fields, photo, high-fidelity profile clone, UID/visible-phone cached-peer search, native collection and item galleries, featured gift, pinned gifts, and fourteen app-icon choices.
- Collectible gallery: the collection screen uses native artwork tiles, each collection opens a scrollable exact-item grid, and clicking a collectible selects it without leaving IntelGram.
- Local gift recipient: featured, pinned, and cloned collectible details match local gift slugs immediately, use your active real or local display name for **Gifted to** and the **Telegram** profile chip, and open your own locally rendered profile without changing real ownership data.
- Compact profile consistency: the collectible's Telegram row now opens a card using your local display name, large profile photo, phone, usernames, bio, and personal channel instead of the collectible owner's server profile.
- Collectible owner consistency: collectible username and anonymous-number boxes now show your active local or cloned display name beside the locally rendered avatar instead of the underlying Telegram peer label.
- Clone fidelity: premium and verification badges, organization badge symbols, emoji status, and personal channel follow a read-only refresh of the already-known source profile; absent elements are cleared from the cloned local view.
- Username editor: the added original-username strip and visible local-only disclaimer are gone; the dialog now matches Telegram's native title, `@username` field, validation row, help copy, spacing, and buttons without a network availability or account-update request.
- Settings organization: the home page is grouped into Customize, Power tools, and Client settings; Local Profile, Appearance, Navigation, Vault, Automation, Export, and Advanced controls each have focused pages.
- Dynamic preview: the name-color sample follows the currently rendered local display name.
- Icon pack: pink primary and profile-art icons plus twelve color variants include native macOS, multi-resolution Windows, and Linux resources; macOS artwork is full-bleed and no longer receives a second white frame or inset.
- Settings cleanup: inherited Boosty and cryptocurrency donation rows were removed and replaced with IntelGram community, source, and changelog links.
- Community badge: joining `@intelgrams` through Telegram's normal channel page unlocks a local supporter badge; IntelGram performs no automatic join request.
- Native local ownership: open a broadcast channel's three-dot menu, choose **IntelGram -> Local ownership**, and enable an owner-style view in that channel itself.
- Native channel composer: **Broadcast a message...** stores text in IntelGram's local vault and renders it in the channel's real timeline without calling Telegram's send API.
- Owner controls: native **Manage channel** actions open the local ownership panel, while contradictory Join, Leave, and Report rows are hidden until the mode is disabled.
- Local post boundary: media, files, voice messages, stickers, inline results, forwards, and scheduled posts are blocked before Telegram upload or send paths.
- Local channel showcase: choose a public `@channel` or `t.me` link to render it on your own profile with an explicit **Local showcase** label; lookup is read-only and does not claim ownership, join the channel, grant permissions, or modify Telegram.
- Local channel workspace: open the showcase as a private local feed, create text posts with optional local-file attachments, edit/delete/pin/copy them, and configure a local title, description, and photo without publishing or modifying a Telegram channel.
- In-app update log: **IntelGram Preferences -> Update log** opens a bundled native summary without requiring GitHub.
- Current source patch SHA-256: `d9ecc14859dc68f055abdc69e1a27000a5b8df62be99535d131a389edd07561e`.
- Privacy boundary: no Telegram profile mutation, contact import, automatic channel join, channel ownership or permission mutation, collectible transaction, or protected-content bypass.
- Protected content: Restrict Saving Content and self-destructing items remain metadata-only jump-back references; bodies, links, filenames, revisions, and media are excluded from rules, exports, and backups.
- Branding consistency: product-facing window, settings, version, and notification-preview titles use IntelGram while upstream attribution remains intact.
- Login and platform branding: the login footer, application menus, About and crash dialogs, tray labels, updater identity, and Windows shortcut metadata now consistently use IntelGram.
- macOS glass reliability: the native blur material is ordered as a sibling behind Qt's native content view, keeping text, controls, avatars, and media visible when transparent mode is enabled.
- Windows reliability: the final Qt dependency stage pre-creates generated module metadata directories and runs serialized to prevent the `qtimageformats` race seen in the previous run.

## Validation Recorded For Each Release

- Patch SHA-256 verification before application.
- Clean `git apply --check` against the pinned official source revision.
- Whitespace and postimage checks.
- Scan for unexpected Telegram profile mutation references.
- Scan for unexpected channel-join requests.
- Verify protected-content and native-forwarding guard hooks.
- Verify Qt 6.2-compatible APIs and required vault, export, native theme-gallery, and custom-background hooks.
- Platform configure and compile result.
- Package identity and executable checks.
- Isolated-work-directory launch smoke test.
- Package SHA-256 checksums and launch logs.

## Release Outputs

- `IntelGram-macOS-Apple-Silicon.dmg`
- `IntelGram-macOS-Apple-Silicon.zip`
- `IntelGram-Windows-x64.zip`
- `IntelGram-Linux-x64.tar.gz`

The newest launch-tested packages, checksums, and validation reports are always attached to the [latest IntelGram release](https://github.com/foolspec/IntelGram/releases/latest).
