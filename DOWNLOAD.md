# IntelGram Downloads

IntelGram is built from official AyuGram Desktop v6.7.8 with the local profile, collectible, Vault & Tools, theme, and export suite.

## Download

- [macOS Apple Silicon DMG](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-macOS-Apple-Silicon.dmg)
- [macOS Apple Silicon app ZIP](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-macOS-Apple-Silicon.zip)
- [Windows x64 ZIP](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Windows-x64.zip)
- [Linux x64 tar.gz](https://github.com/foolspec/IntelGram/releases/latest/download/IntelGram-Linux-x64.tar.gz)
- [Checksums, validation logs, and release notes](https://github.com/foolspec/IntelGram/releases/latest)

Each release attaches platform-specific SHA-256 files, validation notes, and launch logs. The [release validation workflow](https://github.com/foolspec/IntelGram/actions/workflows/intelgram-release-validation.yml) downloads the published assets and verifies their digests.

The application is named `IntelGram`. Its distinct bundle and application IDs let it coexist with a normal AyuGram installation. The macOS package is ad-hoc signed, so macOS may require Control-click, then **Open**, on first launch.

After signing in, open **IntelGram Settings -> Other -> Local profile** for local identity and collectible rendering. Open **IntelGram Settings -> Vault & Tools** for local search, timelines, smart folders, private notes, rules, per-chat controls, themes, exports, and encrypted backup.

The main search field can also find already-loaded peers by UID or a phone number already visible to your account. None of these additions mutate Telegram profile data, import contacts, join a channel, change collectible ownership, or bypass Restrict Saving Content.
