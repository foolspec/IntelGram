# Security Policy

## Supported versions

| Version | Supported |
| ------- | --------- |
| Latest release (v21) | Yes |
| Older releases | No |

Please update to the latest release before reporting an issue — older builds may already be fixed.

## Reporting a vulnerability

IntelGram is a local-only client modification: it does not run servers, collect accounts, or store your data anywhere. Still, if you find a security issue (for example, local data leaking where it should not, or a way for the app to send a Telegram request it claims not to send), please report it.

- Open an issue on [GitHub](https://github.com/foolspec/IntelGram/issues) with a brief description and the version you tested, or message [@intelgrams](https://t.me/intelgrams) on Telegram.
- Please do not post exploit details or proof-of-concept code in public issues. Describe the impact and the maintainer will follow up for details.
- Reports are reviewed as quickly as possible. If the issue is confirmed, a fix will be released and credited in the changelog unless you prefer to stay anonymous.

## What is out of scope

- Issues in upstream AyuGram Desktop or Telegram Desktop — please report those to their own projects.
- The simulated local Stars balance and demo checkout: these are intentionally local-only and involve no real payment, so "free Stars" in your own client is the feature, not a vulnerability.
- Screenshots showing locally rendered profile overrides: local rendering is visible to whoever holds the device by design.
