# IntelGram Feature Guide

## Where do I change my local profile?

Open **IntelGram Preferences -> Local Profile & Collectibles**. The controls are grouped into **Identity**, **Usernames, bio and contact**, **Profile photo**, and **Collectibles**. Each field has its own switch or action, so you can use only the pieces you want.

## How do I change my local username?

Enable local usernames, then click your username. The editor intentionally looks like Telegram's normal **Username** dialog, including its `@username` field, inline status, help copy, spacing, and buttons. It validates the local value's Telegram-style syntax and marks a valid value as available inside IntelGram, but does not ask Telegram whether the public username is available and does not update the account. Add the primary rendered username and, if needed, up to 20 other usernames.

## How do I use an anonymous number or local UID?

Enable the corresponding setting and enter the value you want rendered. These are display overrides inside IntelGram, not Telegram account changes.

## How do I use a local profile picture?

Choose **Local profile photo**, select an image on this Mac or PC, and enable the local profile presentation. Clear the photo setting to restore the normal Telegram image.

## How do I clone a profile locally?

First open the other user's profile so IntelGram has loaded it. Copy their UID, choose **Clone profile locally by UID**, paste the UID, and confirm. IntelGram refreshes that already-known profile read-only, then mirrors only data visible to you, including premium or verification badges, the organization badge symbol, emoji status, and personal channel. If the source has none of those elements, the cloned local view shows none even when your real profile has one. Choose **Stop cloning profile** to return to your own local fields.

## How do I show a channel on my local profile?

Under **Local Profile & Collectibles**, enable **Show a public channel on my local profile**, choose **Choose showcase channel**, and enter a public `@channel` or `t.me` link. IntelGram resolves the public channel read-only and shows it in the profile row labeled **Local showcase**. This is presentation only: it does not make you the owner or an administrator, join the channel, grant permissions, or change Telegram.

## How do I post in the local channel?

Click the **Local showcase** row on your profile or choose **Open local channel** in Local Profile settings. The **Local-only channel** workspace has **New local post** and **Channel settings** actions. A local post can contain text and one local-file attachment; click a post to edit, pin, copy, open its attachment, or delete it. Channel settings can override the title, description, and photo shown inside the workspace.

These are private IntelGram records, not Telegram messages. They do not publish to the selected channel, create ownership or administrator rights, alter public channel settings, affect subscribers, or appear to anyone else.

## How do I use local ownership in a real channel view?

Open any broadcast channel, click the three-dot menu, open **IntelGram**, and choose **Local ownership**. Turn on **Enable local ownership**. The same channel screen now shows Telegram's native **Broadcast a message...** composer, a local-only bell control, and **Manage channel** actions. Type a text post and send it normally; IntelGram intercepts it before Telegram's API and inserts it into that channel's timeline as a persistent local client message.

Right-click a local broadcast to use the native reaction strip and the owner-style Reply, Edit, Pin/Unpin, Copy Text, Copy Post Link, Forward, Delete, and Select rows. Replying through the native composer, editing, pinning, reactions, views, and deletion all update the saved local record. The Forward row makes a safe text handoff and never passes the synthetic message ID to Telegram.

Open **Broadcast engagement** in the ownership settings to choose starting and maximum views, the increment and interval, six reaction totals, and a paid-Star display count. These counters are simulated only in this IntelGram installation. Use **Clear local broadcasts** to remove every local post, or turn off local ownership to return to the ordinary subscriber view. Media, files, voice messages, stickers, inline results, synthetic-message forwards, and scheduled posts are intentionally blocked so they cannot upload or become Telegram channel activity.

## How do I find somebody by UID or phone?

Use the normal search field above the chat list. Paste a UID, `id: UID`, or a phone number. A known matching profile appears under **Found by ID or phone**. This cannot discover strangers or hidden phone numbers; the peer must already be loaded and the phone must already be visible.

## How do I browse collectible gifts?

Choose **Choose featured collectible** or **Manage pinned collectible gifts**. The first screen is a native gallery of collection artwork. Click a collection, scroll through its numbered collectibles, and click the exact artwork you want; more items load as you reach the end. The entire selection flow stays inside IntelGram.

## Can I paste a Getgems or TON link?

Yes. Use **Open by collectible link** and paste a supported Getgems item URL, `t.me/nft` URL, Telegram gift slug, or TON NFT address. IntelGram resolves the exact Telegram collectible when the metadata maps to one.

## How do I change the IntelGram app icon?

Open **IntelGram Preferences -> Appearance & Backgrounds**, then choose **App Icon**. Choose the pink primary artwork, the pink profile-art alternate, or one of twelve color variants. IntelGram stores the choice locally and uses the matching platform-native icon resource. On macOS, the Dock icon uses full-bleed artwork without an extra white frame.

## Why does the name-color preview show a different name?

The preview follows the name IntelGram currently renders for your own profile. Change the enabled local display name and the sample updates to that value instead of showing a fixed name.

## What happens when I click a rendered collectible?

IntelGram opens Telegram's native unique-gift detail surface for that exact model and number. For a locally selected or cloned collectible, **Gifted to** and the **Telegram** profile chip use the display name IntelGram currently renders for you; clicking either opens your own locally rendered profile card with the active local photo, phone, usernames, bio, and personal channel. The local presentation does not claim ownership, rewrite the real sender or date, or create a transaction.

## Can anybody else see these changes?

No. They are visible only in this IntelGram installation. Screenshots will naturally show what IntelGram rendered, but Telegram and other users receive none of these local values.

## How do I get the IntelGram supporter badge?

Open **IntelGram Preferences -> IntelGram -> Telegram**, then press Telegram's normal **Join** button on `@intelgrams`. IntelGram reads that locally known membership state and displays the badge on your own profile. It does not join the channel automatically, and leaving the channel removes the local membership-derived badge after Telegram updates the channel state.

## Where can I read the update log?

Open **IntelGram Preferences -> Update log**. The bundled dialog summarizes the latest update, main IntelGram features, and the local-only privacy boundary. **View full changelog** opens the longer GitHub history when you need it.

## Where is the message vault?

Open **IntelGram Preferences -> Vault & Search**. **Search this account** searches the active account; **Search all accounts** and **Unified account inbox** label results with the account they belong to. IntelGram starts indexing messages as this build receives or loads them, so older history appears as it is loaded.

## How do I use the timeline tools?

Open a chat, then choose **Timeline & conversation stats**. From there you can inspect totals, jump to a date, browse compact media history and revisions, or open saved moments. Results return to the original message when it is still available.

## How do I save a moment?

Right-click a message and choose **Save as IntelGram moment**. Moments are private local bookmarks with optional notes and tags. Protected chats create a jump-back reference only.

## How do I add private contact notes?

Open a person's chat, then choose **Private note for current contact** under **Contacts & Automation**. You can store a note, tags, relationship/context text, and an optional reminder date. **All private profile notes** shows the saved list and due reminders.

## What does the identity inspector store?

It takes an opt-in snapshot of public fields already visible to IntelGram: display name, usernames, public badge flags, and shared-group count. It does not query hidden fields or continuously monitor strangers.

## How do local rules work?

Choose **Rules & anti-spam**, select a trigger such as a keyword, link, photo, or file, then choose a local action. Rules can tag, save a moment, add an alert, mute locally, send to spam review, or queue an ordinary message for manual forwarding. A forward queue still opens Telegram's normal recipient picker and never accepts protected content.

## How do I configure one chat?

Open the chat and choose **Current chat tools**. Set tags, priority, download mode, read reminder, local-only draft preference, or local notification mute. These settings affect IntelGram on this device and do not rewrite the chat's server settings.

## How do I export a chat or account?

Open **IntelGram Preferences -> Export & Backup** and choose the current chat or current account. IntelGram writes HTML, PDF, Markdown, JSON, and ZIP files to the folder you select. Select messages in a chat and use **Export selected messages** to export only that selection.

## What is Frozen Account Backup?

It creates one encrypted `.intelvault` archive containing IntelGram's structured local export and permitted cached files. Encryption uses the passphrase you enter; keep it safe because IntelGram does not upload or recover it. Telegram login keys and session credentials are deliberately excluded.

## What happens in a restricted channel?

IntelGram preserves Telegram's Restrict Saving Content and self-destruct rules. Search, moments, rules, exports, and backups keep only basic metadata and a jump-back reference. The protected message body and media are not copied, forwarded, or packaged.

## How do I change themes?

Open **IntelGram Preferences -> Appearance & Backgrounds**. Choose from the visual light and dark theme cards, browse Telegram cloud themes inside the app, or select the chat-background preview to open Telegram's wallpaper gallery. The background editor can apply a wallpaper from the gallery or a local image and includes preview, tiling, and wide-layout controls. Theme, palette, and supported image files can also be imported through the native file picker. To remove an installed theme, select it and choose **Remove current custom theme**; built-in defaults are protected.

## How do I make the whole app transparent or more animated?

Open **IntelGram Preferences -> Glass & Motion**. Enable Liquid Glass, choose which surface groups use the material, and adjust tint, opacity, adaptive color, moving highlights, edge refraction, and optical intensity. Native desktop blur is optional; you can instead choose a removable local image for the whole app and set its opacity and blur. The material uses cached blur, light diffusion, moving reflections, fine grain, and refractive edges while leaving text and media opaque.

Enable Enhanced Animations separately, then choose a pack: **Quick Snap** is compact and immediate, **Smooth Flow** is calm and critically damped, **Diabolical** is brisk with stronger depth, **Springy** settles gently without overshoot, and **Liquid Bounce** is the most elastic with a controlled overshoot. Both master switches can be turned off at any time, and reduced-motion or power-saving mode still takes priority.

## Where did the other settings go?

The IntelGram Preferences home page is grouped into **Customize**, **Power tools**, and **Client settings**. Navigation and tray behavior are in **Navigation & Layout**; contact notes, rules, identity inspection, privacy, and anti-spam tools are in **Contacts & Automation**; exports and Frozen Account Backup are in **Export & Backup**; diagnostics, URL registration, and reset controls are in **Advanced & Maintenance**.
