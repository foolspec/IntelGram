# IntelGram Features

This document covers IntelGram's custom additions. IntelGram also retains the upstream AyuGram Desktop and Telegram Desktop feature sets.

## Local Profile Fields

- Local display name with a dedicated enable switch.
- Local UID with a dedicated enable switch.
- Local primary username and up to 20 other local usernames.
- Telegram-style inline syntax and availability status in the local username editor, with no server-side availability request.
- Local anonymous number.
- Local bio.
- Local profile photo selected from a file on this device.
- Persistent local settings that survive an IntelGram restart.
- One action to clear each local value and return to the real Telegram-rendered value.

## Local Profile Clone

- Accepts the UID of a user whose profile has already been opened and loaded in IntelGram.
- Uses Telegram's standard read-only full-profile refresh for that already-known user so visible clone metadata is current.
- Mirrors the loaded user's visible name, UID, primary and other usernames, visible phone, bio, profile photo, profile colors, premium or verification state, organization badge symbol, emoji status, personal channel, and featured collectible.
- Clears each cloned badge, status, and personal-channel element locally when the source profile does not have it, instead of falling back to the real self profile.
- Applies the mirror only to your own local profile rendering.
- Can be stopped at any time to return to the individual local fields.
- Does not fetch a hidden profile, bypass privacy, alter either account, or impersonate the user to anyone else.

## UID And Phone Search

- Paste a numeric UID, `id: UID`, or `id UID` into the normal chat-list search field.
- Paste a visible phone number with common spaces, parentheses, periods, or dashes.
- Numeric text attempts both UID and phone matching, with duplicate results removed.
- Results use the existing dialog peer row and open the normal profile when selected.
- Lookup is limited to peers already loaded by IntelGram and phone numbers already visible to your account.
- Recognized UID and phone searches bypass remote message, topic, and username search requests.
- No contact import, address-book sync, username resolution bot, or profile mutation is added.

## Collectible Browser

- Opens directly into a native visual gallery of collection thumbnails inside IntelGram.
- Loads all collection types currently returned by Telegram's read-only star-gift catalog.
- Falls back to read-only sample lookups for Scared Cat, Plush Pepe, Toy Bear, Chill Flame, Precious Peach, Heart Locket, Diamond Ring, and Astral Shard when the catalog omits collection tiles.
- Opens a scrollable native item grid for the selected collection.
- Loads exact numbered collectibles in pages as the user scrolls.
- Selects a collectible by clicking its artwork, without opening Getgems or leaving IntelGram.
- Accepts a collection number, Telegram gift slug, `t.me/nft` link, Getgems item URL, friendly or raw TON NFT address.
- Resolves exact model, pattern, backdrop, number, and native artwork read-only.
- Uses Telegram's native unique-gift detail surface when a rendered collectible is clicked.
- Presents locally selected and cloned gifts as gifted to your currently rendered self name, with both the recipient link and Telegram profile chip opening your own locally rendered profile.
- Matches configured gift slugs before asynchronous collectible IDs resolve, preventing local items from opening the original owner's profile.
- Applies the local name, large profile photo, phone, usernames, bio, and personal channel to the compact profile card opened from the gift detail.
- Supports one local featured collectible and up to six local pinned collectibles.
- Uses the featured collectible for the local profile backdrop and places pinned gifts around the local avatar.
- Never changes ownership, transfers, lists, purchases, upgrades, pins, or features a gift on Telegram.
- Leaves real Telegram and on-chain gift ownership, sender, date, and transaction metadata untouched.

## Universal Message Vault

- Stores a private SQLite index under IntelGram's own work directory.
- Uses SQLite FTS5 for local full-text search, with a compatible fallback when FTS5 is unavailable.
- Indexes received message text, filenames, media types, and links across signed-in accounts.
- Provides current-account search, all-account search, and a unified inbox with clear account labels.
- Stores locally observed revisions before an edit is applied and records deletions only for messages this client had already received.
- Shows protected messages only as metadata and jump-back references; their body, filename, link text, and cached media path are blank.
- Can be turned off from **Vault & Tools** without changing Telegram history.

## Chat Timeline Tools

- Jump to a date in the currently open chat.
- View indexed message, media, link, edit, deletion, and protected-reference counts.
- Browse compact media history.
- Browse locally observed message revision history.
- Save and remove private moments with a title, note, and tags.
- Open a result, revision, or moment back at its original Telegram message when it is still available.

## Smart Folders And Multiple Accounts

- **Unread from real people** filters unread incoming person-to-person messages.
- **Work**, **High priority**, **Needs reply**, and **Spam review** use local tags and chat policy.
- Unified search and inbox results carry the signed-in account's display name.
- Rules and chat policies are account-scoped, so the same dialog ID on another account remains separate.

## Private Contact Context

- Attach private notes, tags, relationship/context text, and an optional reminder date to a loaded contact.
- Keep a local history of note changes.
- Show due reminders from the Vault & Tools page.
- Capture opt-in identity snapshots containing only the public name, usernames, public badge flags, and shared-group count already visible to IntelGram.
- Never fetch hidden profile fields or send notes to Telegram.

## Local Rules And Anti-Spam

- Match a keyword, any link, a photo, or a file on newly indexed messages.
- Add a local tag, save a moment, add local activity, mute the chat locally, send the item to spam review, or queue an ordinary message for manual forwarding.
- Forward queues always open Telegram's native confirmation picker and require `allowsForward()` at execution time.
- Protected messages cannot enter a forward queue and remain reference-only.
- Unknown non-bot senders and suspicious invite links can be routed to local review tags.
- Telegram's existing report and block controls remain available from normal chat/profile menus.

## Per-Chat Controls

- Private tags and priority from 0 through 3.
- Download modes: Telegram default, manual, Wi-Fi/Ethernet only, or always.
- Local read-reminder preference.
- Local-only draft preference that prevents that chat's draft from being uploaded by IntelGram.
- Local notification mute without changing Telegram's server-side notification settings.

## Theme Studio

- Bundled Windows 93, Terminal, Classic Telegram, and true-black AMOLED themes.
- Import an existing `.tdesktop-theme` or `.palette` file.
- Keep the existing IntelGram app-icon chooser with pink character art and twelve color variants.

## Export Center And Frozen Account Backup

- Export the current chat or current account to HTML, PDF, Markdown, JSON, and ZIP.
- Export selected messages directly from the message context menu.
- Include permitted locally downloaded files inside ZIP archives without exposing host filesystem paths in JSON.
- Include contacts already visible to the account, profile notes and history, identity snapshots, rules and activity, smart-folder policies, options, moments, and revisions in account exports.
- Create one `.intelvault` file using AES-256-GCM with PBKDF2-HMAC-SHA256 and 250,000 iterations.
- Stream ZIP creation and encryption instead of loading a complete backup into memory.
- Avoid leaving plaintext export files behind when encrypted mode is selected.
- Exclude authorization keys, Telegram session credentials, protected media, and self-destructing content.

## Render Coverage

- Settings account cover and profile rows.
- Main menu identity and account switcher rows.
- Dialog list rows, avatars, video-userpic fallbacks, and search results.
- Chat headers, profile headers, top bars, about sections, service text, messages, forwarded/reply previews, and table rows.
- Own-profile username, UID, phone, bio, photo, colors, premium and verification badges, organization badge symbol, emoji status, personal channel, featured gift, backdrop, and pinned-gift visuals.
- Native collectible tooltip and detail interaction, including self-recipient rendering for local and cloned gifts.
- Collectible username and anonymous-number owner chips use the active local or cloned display name alongside the locally rendered avatar.

## Product And Packaging

- Visible IntelGram product name.
- In-app links to `@intelgrams`, the IntelGram GitHub repository, and both changelogs.
- Bundled native update log covering the latest changes, main features, privacy boundary, and project credit.
- Optional local supporter badge after the user explicitly joins `@intelgrams` through Telegram's channel page.
- Grouped local-profile controls for identity, contact details, profile photo, and collectibles.
- A reactive name-color sample that follows the locally rendered display name.
- New pink IntelGram primary and profile-art icons plus twelve color variants, with full-bleed macOS artwork and platform-native Windows and Linux resources.
- Separate macOS bundle identifier, Windows application identifier, and Linux desktop identifiers.
- Packages for macOS Apple Silicon, Windows x64, and Linux x64.
- Coexists with a normal AyuGram installation without overwriting it.
- Automated clean-source patch verification, mutation-reference scan, launch smoke tests, checksums, and release packaging.

## Network And Protected-Content Boundary

IntelGram's custom profile values stay local. Clone selection may issue Telegram's standard read-only full-profile refresh for the already-known source user. The collectible browser may issue read-only Telegram gift catalog/detail requests and a read-only TonAPI metadata lookup when resolving a raw TON NFT address. The supporter badge reads an already-known channel membership state and does not join a channel. Vault rules run only after IntelGram receives a message.

IntelGram adds no Telegram account/profile update, contact import, collectible transaction, ownership mutation, automatic channel join, or protected-content bypass. Restrict Saving Content and self-destruct flags are enforced in the vault, edit/deletion history, saved moments, rules, exports, cached-media packaging, media overlay, and legacy Ayu message storage.

## Credit

IntelGram custom features by **fool**. Upstream AyuGram Desktop and Telegram Desktop attribution and licenses are preserved.
