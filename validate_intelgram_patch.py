#!/usr/bin/env python3

from __future__ import annotations

import pathlib
import subprocess
import sys

BASELINE = "b25513a06ff88be0b3f4c928252b56c3da39cec7"
LIB_UI_COMMIT = "b9a30917daf2bd8fdc17ccd9682acca178882b7b"


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
            "ayu/ui/settings/settings_visual_effects.cpp",
            "ayu/ui/local_channel_workspace.cpp",
            "ayu/ui/visual_effects.cpp",
        ),
        "Telegram/Resources/langs/lang.strings": (
            '"ayu_CategoryAppearance" = "Appearance & Backgrounds";',
            '"ayu_RemoveCurrentTheme" = "Remove current custom theme";',
            '"ayu_VisualEffectsTitle" = "Glass & Motion";',
            '"ayu_VisualEffectsEnableGlass" = "Enable Liquid Glass";',
            '"ayu_VisualEffectsMotionDynamic" = "Diabolical";',
            '"ayu_VisualEffectsMotionLiquidBounce" = "Liquid Bounce";',
            '"ayu_LocalProfileChannelProfileLabel" = "Local showcase";',
            '"ayu_LocalChannelWorkspaceTitle" = "Local-only channel";',
            '"ayu_LocalChannelWorkspacePostSave" = "Post locally";',
            '"ayu_LocalChannelWorkspaceSettingsTitle" = "Local channel settings";',
            '"ayu_VaultTitle" = "Vault & Search";',
            '"ayu_AutomationTitle" = "Contacts & Automation";',
            '"ayu_ExportTitle" = "Export & Backup";',
            '"ayu_VaultExportSelectedMessages" = "Export selected messages";',
            "Restrict Saving Content",
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
            "ShowExport(",
            "AyuAutomation::Id()",
            "AyuExport::Id()",
        ),
        "Telegram/SourceFiles/ayu/ui/settings/settings_appearance.cpp": (
            "SetupThemeOptions(",
            "SetupCloudThemes(",
            "SetupChatBackground(",
            "RemoveCurrentTheme(",
            "cloudThemes().remove(",
            "AyuNavigation::Id()",
        ),
        "Telegram/SourceFiles/ayu/ui/settings/settings_main.cpp": (
            "tr::ayu_SettingsCustomizeHeader()",
            "AyuVisualEffects::Id()",
            "AyuAutomation::Id()",
            "AyuExport::Id()",
            "AyuAdvanced::Id()",
        ),
        "Telegram/SourceFiles/ayu/ui/settings/settings_visual_effects.cpp": (
            "AyuVisualEffects::Id()",
            "AyuSettings::transparentMode",
            "AyuSettings::transparentSidebar",
            "AyuSettings::transparentChat",
            "AyuSettings::transparentPanels",
            "AyuSettings::transparentAdaptiveMaterial",
            "AyuSettings::transparentDynamicHighlights",
            "AyuSettings::transparentRefraction",
            "setTransparentMaterialIntensity(value);",
            "AyuSettings::enhancedAnimations",
            "AyuSettings::animateWindowOpening",
            "ChooseBackdrop(",
        ),
        "Telegram/SourceFiles/ayu/ui/visual_effects.cpp": (
            "Images::BlurLargeImage",
            "Platform::SetWindowVisualEffect",
            "anim::SetMotionStyle",
            "AdaptiveTint(",
            "EdgePath(",
            "WindowAnimationEnabled(",
            "void PaintWindow(",
        ),
        "Telegram/SourceFiles/ayu/ui/settings/settings_other.cpp": (
            "ShowLocalProfileChannelBox(",
            "MTPcontacts_ResolveUsername(",
            "settings.setLocalProfileChannelEnabled(true);",
            "Ayu::ShowLocalChannelWorkspace(controller);",
            "ayu_LocalProfileChannelLocalOnly",
        ),
        "Telegram/SourceFiles/ayu/ui/local_channel_workspace.cpp": (
            "localProfileChannelWorkspace()",
            "setLocalProfileChannelWorkspace(encoded)",
            "MTPcontacts_ResolveUsername(",
            "ShowLocalChannelWorkspace(",
            "ShowPostComposer(",
            "ShowPostActions(",
            "ShowChannelSettings(",
            "kMaxLocalChannels = 8",
            "kMaxLocalChannelPosts = 100",
            "File::Launch(post.attachment)",
            "LocalChannelDisplayTitle(",
        ),
        "Telegram/SourceFiles/info/profile/info_profile_actions.cpp": (
            "LocalProfileChannelShowcaseActive(user)",
            "ayu_LocalProfileChannelProfileLabel",
            "Ayu::ShowLocalChannelWorkspace(window);",
            "Ayu::LocalChannelDisplayTitle(channel)",
        ),
        "Telegram/SourceFiles/ayu/ui/utils/ayu_profile_values.cpp": (
            "void ShowLocalProfileUsernameEditor(",
            "box->setTitle(tr::lng_username_title());",
            "class LocalUsernameEditor final : public Ui::RpWidget",
            "object_ptr<Ui::UsernameInput> _field;",
            "rpl::event_stream<> _changes;",
            "rpl::event_stream<> _submits;",
            "&Ui::MaskedInputField::changed",
            "&Ui::MaskedInputField::submitted",
            "_padding(st::usernamePadding)",
            "object_ptr<LocalUsernameEditor>(",
            'rpl::single(u"@username"_q)',
            "editor->changes()",
            "editor->submits(",
            "tr::lng_username_description1(tr::rich)",
            "tr::lng_username_description2(tr::rich)",
            "settings.setLocalProfileUsername(field->getLastText());",
            "settings.setLocalProfileUsernameEnabled(true);",
        ),
        "Telegram/SourceFiles/platform/mac/specific_mac.mm": (
            "NSVisualEffectView",
            "NSVisualEffectBlendingModeBehindWindow",
            "void SetWindowVisualEffect(",
        ),
        "Telegram/SourceFiles/platform/win/specific_win.cpp": (
            "DwmSetWindowAttribute",
            "DwmEnableBlurBehindWindow",
            "void SetWindowVisualEffect(",
        ),
        "Telegram/SourceFiles/platform/linux/specific_linux.cpp": (
            "void SetWindowVisualEffect(",
        ),
        "Telegram/lib_ui/ui/effects/animation_value.cpp": (
            "MotionDuration(",
            "MotionTransition(",
            "MotionStyle::Springy",
            "MotionStyle::LiquidBounce",
        ),
        "Telegram/lib_ui/ui/effects/animations.h": (
            "anim::MotionDuration(duration)",
            "anim::MotionTransition(std::move(transition))",
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

    lib_ui_commit = subprocess.run(
        ["git", "-C", str(root / "Telegram/lib_ui"), "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    if lib_ui_commit != LIB_UI_COMMIT:
        fail(
            "Telegram/lib_ui is not at the pinned IntelGram motion commit "
            f"{LIB_UI_COMMIT}"
        )

    removed_themes = (
        "Telegram/Resources/intelgram-windows93.tdesktop-theme",
        "Telegram/Resources/intelgram-terminal.tdesktop-theme",
        "Telegram/Resources/intelgram-amoled.tdesktop-theme",
    )
    for path in removed_themes:
        if (root / path).exists():
            fail(f"removed novelty theme is still present: {path}")

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
        "MTPaccount_CheckUsername",
        "MTPaccount_UpdateProfile",
        "MTPaccount_UpdateUsername",
        "MTPaccount_UpdateEmojiStatus",
        "MTPaccount_UpdatePersonalChannel",
        "MTPchannels_JoinChannel",
        "MTPchannels_CreateChannel",
        "MTPchannels_DeleteChannel",
        "MTPchannels_EditAdmin",
        "MTPchannels_EditPhoto",
        "MTPchannels_EditTitle",
        "MTPchannels_InviteToChannel",
        "MTPcontacts_ImportContacts",
        "MTPmessages_EditMessage",
        "MTPmessages_SendMessage",
        "MTPmessages_UpdatePinnedMessage",
        "MTPpayments_TransferStarGift",
        "MTPpayments_ToggleStarGiftsPinnedToTop",
    )
    for needle in forbidden:
        if needle in added:
            fail(f"unexpected Telegram mutation reference {needle}")
    profile_values = (
        root
        / "Telegram/SourceFiles/ayu/ui/utils/ayu_profile_values.cpp"
    ).read_text(encoding="utf-8", errors="strict")
    language = (
        root
        / "Telegram/Resources/langs/lang.strings"
    ).read_text(encoding="utf-8", errors="strict")
    for obsolete in (
        "ayu_LocalProfileUsernameEditorTitle",
        "ayu_LocalProfileUsernameEditorDescription",
    ):
        if obsolete in profile_values or obsolete in language:
            fail(f"username editor still exposes obsolete local copy {obsolete}")
    if "QNetworkInformation" in added:
        fail("Qt 6.3-only QNetworkInformation API is not compatible with the pinned Qt 6.2 build")

    print("IntelGram patch validation passed.")


if __name__ == "__main__":
    main()
