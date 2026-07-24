#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import subprocess
import sys

BASELINE = "b25513a06ff88be0b3f4c928252b56c3da39cec7"


def fail(message: str) -> None:
    raise SystemExit(f"IntelGram patch validation failed: {message}")


def require(root: pathlib.Path, path: str, needles: tuple[str, ...]) -> None:
    target = root / path
    if not target.is_file():
        fail(f"missing {path}")
    text = target.read_text(encoding="utf-8", errors="strict")
    for needle in needles:
        if needle not in text:
            fail(f"{path} does not contain {needle!r}")


def main() -> None:
    if len(sys.argv) != 2:
        fail("usage: validate_intelgram_patch.py SOURCE_ROOT")
    root = pathlib.Path(sys.argv[1]).resolve()
    if not (root / ".git").exists():
        fail(f"{root} is not a Git checkout")

    requirements = {
        "Telegram/CMakeLists.txt": (
            "ayu/data/intelgram_vault.cpp",
            "ayu/data/intelgram_export.cpp",
            "ayu/ui/settings/settings_vault.cpp",
        ),
        "Telegram/Resources/langs/lang.strings": (
            '"ayu_VaultTitle" = "Vault & Tools";',
            '"ayu_VaultExportSelectedMessages" = "Export selected messages";',
            "Restrict Saving Content",
        ),
        "Telegram/Resources/qrc/telegram/telegram.qrc": (
            "intelgram-windows93.tdesktop-theme",
            "intelgram-terminal.tdesktop-theme",
            "intelgram-amoled.tdesktop-theme",
        ),
        "Telegram/SourceFiles/ayu/data/intelgram_vault.cpp": (
            "CREATE VIRTUAL TABLE IF NOT EXISTS vault_messages_fts USING fts5",
            "std::vector<SearchResult> MessagesByIds(",
            "bool KeepDraftLocal(",
            "bool SuppressNotification(",
            "item->forbidsSaving()",
            "!item->history()->peer->allowsForwarding()",
        ),
        "Telegram/SourceFiles/ayu/data/intelgram_export.cpp": (
            "EVP_aes_256_gcm()",
            "kArchiveIterations = 250000",
            "Vault::MessagesByIds(",
            "message.protectedContent",
        ),
        "Telegram/SourceFiles/ayu/ui/settings/settings_vault.cpp": (
            "ShowVaultSearch(",
            "ShowTimeline(",
            "ShowRules(",
            "ShowThemeStudio(",
            "ShowExport(",
        ),
        "Telegram/SourceFiles/history/history_item_helpers.cpp": (
            "(flags & MTP::f_noforwards) ? Flag::NoForwards",
        ),
        "Telegram/SourceFiles/history/view/history_view_context_menu.cpp": (
            "tr::ayu_VaultExportSelectedMessages(tr::now)",
            "item->allowsForward()",
        ),
        "Telegram/SourceFiles/data/data_auto_download.cpp": (
            "QNetworkInterface::allInterfaces()",
            "IntelGram::Vault::DownloadMode(peer)",
        ),
        "Telegram/SourceFiles/media/view/media_view_overlay_widget.cpp": (
            "_message->forbidsSaving()",
            "!_history->peer->allowsForwarding()",
        ),
    }
    for path, needles in requirements.items():
        require(root, path, needles)

    diff = subprocess.run(
        ["git", "-C", str(root), "diff", "--unified=0", "--no-ext-diff"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    if not diff:
        diff = subprocess.run(
            [
                "git",
                "-C",
                str(root),
                "diff",
                "--unified=0",
                "--no-ext-diff",
                f"{BASELINE}..HEAD",
            ],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
    added = "\n".join(
        line[1:]
        for line in diff.splitlines()
        if line.startswith("+") and not line.startswith("+++")
    )
    forbidden = (
        "MTPaccount_UpdateProfile",
        "MTPaccount_UpdateUsername",
        "MTPaccount_UpdateEmojiStatus",
        "MTPaccount_UpdatePersonalChannel",
        "MTPchannels_JoinChannel",
        "MTPcontacts_ImportContacts",
        "MTPpayments_TransferStarGift",
        "MTPpayments_ToggleStarGiftsPinnedToTop",
    )
    for needle in forbidden:
        if needle in added:
            fail(f"unexpected Telegram mutation reference {needle}")
    if "QNetworkInformation" in added:
        fail("Qt 6.3-only QNetworkInformation API is not compatible with the pinned Qt 6.2 build")

    print("IntelGram patch validation passed.")


if __name__ == "__main__":
    main()
