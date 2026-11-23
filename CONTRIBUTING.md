# Contributing to IntelGram

Thanks for helping make IntelGram better. This is a small project maintained by one person, so clear reports and small, focused contributions go a long way.

## Reporting bugs

Open an issue on [GitHub](https://github.com/foolspec/IntelGram/issues) and include:

- Your platform and IntelGram version (see **IntelGram Preferences -> Update log**)
- What you expected to happen and what happened instead
- Steps to reproduce, and a screenshot or screen recording if it helps

## Suggesting features

Open an issue and describe the idea, what it would do, and why it fits IntelGram's local-only approach. Features that change your Telegram account, send data to Telegram, or touch other users' accounts are out of scope by design.

## Questions and discussion

Join [@intelgrams](https://t.me/intelgrams) on Telegram.

## Code contributions

A note on how this repository is organized, because it is unusual: the `main` branch holds a patch repository, not the app source. The build clones pristine AyuGram Desktop `v6.7.8` and applies [`intelgram-local-profile-render-overrides.patch`](intelgram-local-profile-render-overrides.patch), which contains the complete IntelGram implementation. See **Building** in the [README](README.md) and [`TECHNICAL_CHANGELOG.md`](TECHNICAL_CHANGELOG.md) for details.

That means:

- Small fixes to docs, scripts, branding, and workflows can be proposed as pull requests directly against `main`.
- Changes to app behavior need to land in the patch file. If you have a source-level change, open an issue first describing it so the patch can be regenerated cleanly — a pull request that edits the patch by hand will almost certainly not apply.
- Before opening a pull request, run [`validate_intelgram_patch.py`](validate_intelgram_patch.py) if your change touches the patch or the build scripts.

By contributing, you agree that your contributions will be licensed under the same license as the project (GNU General Public License v3.0). See [`LICENSE`](LICENSE).
