# IntelGram Update Log

## Current Update: IntelGram v23

- Base source: official AyuGram Desktop `v6.7.8` at `b25513a06ff88be0b3f4c928252b56c3da39cec7` with required submodules.
- Product versioning: IntelGram releases advance sequentially from v1; the current product release is v23, while `v6.7.8` identifies only the upstream AyuGram source.
- Product identity: IntelGram on macOS, Windows, and Linux with separate application identifiers.
- Source delivery: one complete IntelGram patch plus deterministic branding and validation scripts.
- Build delivery: GitHub Actions packages macOS Apple Silicon, Windows x64, and Linux x64.
- Build reliability: macOS and Windows dependency preparation uses bounded cache stages, platform cache creation is serialized, installed Qt source trees are pruned, macOS can resume directly from a completed Qt stage, and Windows-only runs carry every dependency stage through to the packaged application.
- Gift upgrades to collectibles: upgrade a local gift at Telegram's upgrade fee; it gains a model, pattern, backdrop, and sequential serial number and moves into the Collectibles filter.
- Paid reactions: react with Stars from the local balance in Telegram's native picker; reactions persist on the message and credit your own channels' local earnings.
- Channel boosts: local Premium accounts spend boost slots from the native Boost screens, with level, progress, and removal handled locally.
- Giveaways: create Premium or Stars giveaways with the native flow; winners are drawn locally at the end date, results are posted in the channel, and giveaways appear in the channel's boost statistics.
- Business saves stored locally: greeting, away, working hours, intro, location, chatbot, and quick-reply settings persist on-device, including quick-reply message text; sending a local quick reply works.
- Local gift collections: local gifts join collections and can be hidden and pinned from the context menu and the collection editor.
- Gifting friends: a gift sent to a friend appears in that friend's Gifts tab in this client, marked as from you, and can be upgraded there; gifting Premium shows the Premium badge on their profile; gifted Stars are recorded in your Stars history.
- Local Stars activity history: top-ups, gifts, earnings, withdrawals, and conversions are recorded on-device and shown in the My Stars history.
- Withdrawal fix: withdrawn Stars are now deducted from the spendable balance.
- Statistics fix: dollar amounts use the Stars withdrawal rate instead of the TON rate, and Earn Stars options are claimable once per day.
- Premium fix: local subscriptions expire on schedule, and the plan picker charges the plan you selected.
- Convert fix: converting a gift returns 85% of its star price, matching Telegram; upgraded collectibles cannot be converted.
- 1:1 presentation: the Stars, gifts, Premium, and Business flows no longer describe themselves as local or simulated in the app, and the in-app update log covers the 1:1 checkout and the v22 and v23 features.
- Honest limits: gifted Premium is a profile badge only and gifted Stars do not change the recipient's real balance; local collectibles cannot be transferred, resold, or crafted; group-call paid reactions still use Telegram's real flow.
- Payment boundary: no real card details are collected, no payment provider is contacted, and no Telegram payment, gift-transfer, subscription, boost, or giveaway API is invoked.

## IntelGram v22 Local Gifts, Premium & Business

- Base source: official AyuGram Desktop `v6.7.8` at `b25513a06ff88be0b3f4c928252b56c3da39cec7` with required submodules.
- Product versioning: IntelGram releases advance sequentially from v1; the current product release is v22, while `v6.7.8` identifies only the upstream AyuGram source.
- Product identity: IntelGram on macOS, Windows, and Linux with separate application identifiers.
- Source delivery: one complete IntelGram patch plus deterministic branding and validation scripts.
- Build delivery: GitHub Actions packages macOS Apple Silicon, Windows x64, and Linux x64.
- Build reliability: macOS and Windows dependency preparation uses bounded cache stages, platform cache creation is serialized, installed Qt source trees are pruned, macOS can resume directly from a completed Qt stage, and Windows-only runs carry every dependency stage through to the packaged application.
- Gift Stars to friends: the native gift flow deducts from the on-device balance, with the native sent confirmation and insufficient-balance state.
- Buy and send gifts: gifts are bought with local Stars at Telegram's native star prices, including the optional upgrade fee; self-purchases land in the local inventory.
- Local gift inventory: the profile Gifts tab shows locally owned gifts in Telegram's native grid, detail popups, and All Gifts / Collectibles filtering, up to 200 gifts.
- Stars Balance and proceeds: available, total, and lifetime proceeds are fed by local activity, with the 21-day maturity rule and a local Fragment-style withdrawal flow.
- View Statistics: the revenue chart and statistics view are fed by on-device Stars earnings.
- Earn Stars from Mini Apps: Demo Quest, Demo Game, and Demo Ad credit 50, 25, and 10 Stars to the local balance.
- Checkout thumbnail: the Stars checkout invoice now carries a cover thumbnail, matching Telegram's real checkout layout.
- Local Premium purchase: every Subscribe button runs the native plan picker and checkout panel with the demo American Express card; after paying, the Premium badge, subscribed state, and Premium features appear as if the account owns Premium.
- Local Business subscription: the same flow unlocks the Business section and its feature screens.
- Per-account profile clones: each signed-in account keeps its own local profile clone and NFT collections; switching accounts restores that account's clone and collections, and an account with no clone shows its real profile. Your existing clone carries over to the account you are using.
- Per-account Stars, gifts, earnings, and Premium: each signed-in account keeps its own local Stars balance, gift inventory, earnings, and Premium state; Premium bought on one account no longer appears on the others. Your existing balance, gifts, earnings, and Premium carry over to the account you are using.
- Build fix: the Stars panel, gifts tab, and dialogs code now call only APIs present in AyuGram v6.7.8, and the Windows dependency setup no longer installs the removed MSYS2 diffutils package.
- Per-account compile fix: the per-account profile and Stars settings are now mutated through the live per-account entry instead of a copied settings object, and the Premium checkout includes the correct main-window header, so the rebuild compiles on Linux and macOS.
- Final compile fix: the last two pinned-gift-manager settings calls now reach the live per-account entry through a pointer, so the rebuild compiles on Linux and macOS.
- Honest limits: there is no simulated bot chat; Business setting saves are still rejected by Telegram's server without a real subscription; converting a local gift returns an estimated 50% of its star price; gifting Premium to others is untouched.
- Payment boundary: no real card details are collected, no payment provider is contacted, and no Telegram payment, gift-transfer, or subscription API is invoked.

## IntelGram v21 Local Stars & Gifts

- Native My Stars: Stars balance and top-up now live in Telegram's own My Stars screen; Settings shows the local balance on the My Stars row.
- Real Stars packages: Top Up Balance lists Telegram's 15 Stars packages and prices, from 100 to 150,000 Stars.
- Native checkout: tapping a package opens Telegram's checkout panel with the demo American Express card already saved as the payment method; Pay adds the Stars to the on-device balance.
- Payment boundary: no real card details are collected, no payment provider is contacted, and no Telegram payment API is invoked.

## IntelGram v20 Local Stars & Gifts

- Local Stars wallet: a persistent on-device Stars balance appears in a dedicated **Local Stars & Gifts** subsection under Local Profile with a clear local-only disclaimer.
- Demo Amex checkout: a rendered sample American Express card with a fixed number, fixed expiry, editable cardholder name, and **DEMO - AMEX - NO REAL PAYMENT** label accompanies 50/100/250/500/1000 Star presets and a simulated purchase flow.
- Local gift sending: six collectible-style gifts, a recipient label, balance validation, and local deduction replace any real gift transfer API.
- Local history: timestamped purchases and gifts are recorded on-device, with clear-history and full-reset actions.
- Payment boundary: no real card details are collected, no payment provider is contacted, and no Telegram payment, reaction, message-send, or gift-transfer API is invoked.

## IntelGram v19 Exact Telegram Disguise Identity

- macOS disguise stability: global-menu refreshes skip the Ghost Mode actions that are intentionally not created in Telegram disguise mode, preventing the window-activation crash reached by opening `@username` links.
- Username refresh stability: the own-profile QR visibility subscription now retains its peer directly instead of reading a destroyed temporary layout helper.
- Settings search stability: Vault reminder indexing uses the active session and no longer requires a settings window controller, preventing the previous search shortcut quit.
- Exact Telegram identity: the disguised macOS running app and Dock entry are titled **Telegram**, then return to **IntelGram** when the mode is undone.
- Telegram Dock artwork: macOS renders Telegram's full rounded-square source image directly, without the adaptive icon path that previously nested it inside a white tile.
- Reliable Telegram disguise trigger: click anywhere on the blue **IntelGram Desktop v...** heading three times within 1.2 seconds to switch the visible app identity, icons, window title, menus, About surface, and drawer branding to Telegram Desktop while retaining enabled local render features.
- Physical click handling: both the Preferences heading and the drawer restore footer count mouse events directly, so Qt text selection no longer swallows rapid clicks.
- Disguise restore path: open the main drawer and click its **Telegram Desktop** footer three times within 1.2 seconds; the choice is persistent and local.
- Universal vault: local FTS search covers received messages, media metadata, links, and filenames, with current-account, all-account, and unified-inbox views.
- Timeline tools: jump-to-date, conversation statistics, compact media history, saved moments, and locally observed edit/deletion history.
- Organization: smart folders, private contact notes and reminders, note history, opt-in public identity snapshots, and account-scoped local rules.
- Chat controls: local tags, priority, manual/Wi-Fi/always download behavior, read reminders, local-only draft preference, and local notification muting.
- Anti-spam: unknown-sender and suspicious-invite review tags plus keyword, link, photo, and file rules.
- Appearance & Backgrounds: native visual theme cards, in-app cloud themes, imported theme files, a visible remove-current-theme action, Telegram's wallpaper gallery, custom local images, live background preview, tiling controls, and the IntelGram icon set.
- Liquid Glass: cached native or local-image blur with adaptive tint and contrast, wallpaper-colored diffusion, cursor-reactive highlights, moving specular reflections, subtle edge refraction, internal reflections, anti-banding grain, and anti-aliased edges.
- Optical controls: separate switches for adaptive material, dynamic highlights, refractive edges, and an intensity slider, alongside the existing sidebar/chat/panel, tint, opacity, and removable backdrop controls.
- Material performance: slow decorative highlights use a paced 30 Hz refresh while blurred and scaled image results are cached; repainting pauses while hidden, inactive, or reduced motion is enabled, and normal UI animations remain display-synced independently.
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
- Profile badges: Automatic mirrors Premium, verified, SCAM, FAKE, DIRECT, emoji-status, and organization-verification presentation from the cloned or real profile; a focused editor also supports manual local choices.
- Username editor: the added original-username strip and visible local-only disclaimer are gone; the dialog now matches Telegram's native title, `@username` field, validation row, help copy, spacing, and buttons without a network availability or account-update request.
- Settings organization: the home page is grouped into Customize, Power tools, and Client settings; Local Profile, Appearance, Navigation, Vault, Automation, Export, and Advanced controls each have focused pages.
- Dynamic preview: the name-color sample follows the currently rendered local display name.
- Icon pack: pink primary and profile-art icons plus twelve color variants include native macOS, multi-resolution Windows, and Linux resources; macOS artwork is full-bleed and no longer receives a second white frame or inset.
- Settings cleanup: inherited Boosty and cryptocurrency donation rows were removed and replaced with IntelGram community, source, and changelog links.
- Community badge: joining `@intelgrams` through Telegram's normal channel page unlocks a local supporter badge; IntelGram performs no automatic join request.
- Native local ownership: open a broadcast channel's three-dot menu, choose **IntelGram -> Local ownership**, and enable an owner-style view in that channel itself.
- Native channel composer: **Broadcast a message...** stores text in IntelGram's local vault and renders it in the channel's real timeline without calling Telegram's send API.
- Owner controls: native **Manage channel** actions open the local ownership panel, while contradictory Join, Leave, and Report rows are hidden until the mode is disabled.
- Owner post menu: right-click a local broadcast for Telegram's reaction strip plus Reply, Edit, Pin/Unpin, Copy Text, Copy Post Link, Forward, Delete, and Select.
- Persistent post state: local replies, edits, pins, arbitrary emoji or custom-emoji reactions, paid-Star display counts, and client-local post links survive restarts.
- Broadcast engagement: configure starting and maximum views, increment size, interval, six baseline reaction totals, and paid Stars; views advance on-device until the selected maximum.
- Synthetic-message guard: local reactions and Stars never invoke payment or reaction APIs, and Forward creates a safe text handoff instead of passing a local message ID to Telegram.
- Local post boundary: media, files, voice messages, stickers, inline results, forwards, and scheduled posts are blocked before Telegram upload or send paths.
- Local channel showcase: choose a public `@channel` or `t.me` link to render it on your own profile with an explicit **Local showcase** label; lookup is read-only and does not claim ownership, join the channel, grant permissions, or modify Telegram.
- Local channel workspace: open the showcase as a private local feed, create text posts with optional local-file attachments, edit/delete/pin/copy them, and configure a local title, description, and photo without publishing or modifying a Telegram channel.
- In-app update log: **IntelGram Preferences -> Update log** opens a bundled native summary without requiring GitHub.
- Current source patch SHA-256: `f4d2c79e40fd982ef7e8df95594b3df4927822297ab9074d18cb9d0588fb9c64`.
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
