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
            "ayu/ui/discreet_mode.cpp",
            "ayu/ui/local_channel_ownership.cpp",
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
            '"ayu_LocalChannelOwnershipMenu" = "Local ownership";',
            '"ayu_LocalChannelOwnershipToggle" = "Enable local ownership";',
            '"ayu_LocalChannelWorkspaceTitle" = "Local-only channel";',
            '"ayu_LocalChannelWorkspacePostSave" = "Post locally";',
            '"ayu_LocalChannelWorkspaceSettingsTitle" = "Local channel settings";',
            '"ayu_LocalProfileBadges" = "Profile badges";',
            '"ayu_LocalProfileBadgesAutomatic" = "Automatic (Telegram or cloned profile)";',
            '"ayu_LocalProfileBadgesDirect" = "DIRECT";',
            '"ayu_LocalProfileEmojiStatusId" = "Emoji status document ID (optional)";',
            '"ayu_LocalProfileBotVerificationId" = "Organization badge document ID (optional)";',
            '"ayu_IntelGramUpdateLogDisguise" = "**Telegram disguise mode**',
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
            "Ayu::DiscreetModeValue(",
            "kDiscreetModeClickTimeout",
            "base::install_event_filter(labelRaw",
            "QEvent::MouseButtonDblClick",
            "QEvent::MouseButtonRelease",
            "Ayu::SetDiscreetMode(!Ayu::DiscreetModeEnabled())",
            "showOther(MainId());",
            "tr::ayu_IntelGramUpdateLogDisguise(tr::rich)",
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
            "ShowLocalProfileBadgesBox(",
            "Ui::RadioenumGroup<Style>",
            "Style::Automatic",
            "Style::Premium",
            "Style::Verified",
            "Style::Scam",
            "Style::Fake",
            "Style::Direct",
            "mutableSettings.setLocalProfileBadgeStyle(",
            "mutableSettings.setLocalProfileEmojiStatusId(",
            "mutableSettings.setLocalProfileBotVerificationId(",
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
        "Telegram/SourceFiles/ayu/ui/local_channel_ownership.cpp": (
            'u"local_channel_ownership/%1/%2"_q',
            "IntelGram::Vault::Option(key)",
            "IntelGram::Vault::SetOption(key",
            "history->addNewLocalMessage({",
            "MessageFlag::HasViews",
            "MessageFlag::Pinned",
            "MessageFlag::HasReplyInfo",
            "Broadcast engagement",
            "Maximum views",
            "Views added each interval",
            "Paid Stars count",
            "AdvanceViews(",
            "MTP_reactionPaid()",
            "RefreshLocalChannelBroadcastMetrics(",
            "ToggleLocalChannelBroadcastPinned(",
            "ShowEditLocalChannelBroadcast(",
            "IncrementLocalChannelBroadcastReaction(",
            "HandleLocalChannelBroadcast(",
            "InterceptLocalChannelUnsupportedSend(",
            "ShowLocalChannelOwnership(",
            "DeleteLocalChannelBroadcast(",
        ),
        "Telegram/SourceFiles/api/api_views.cpp": (
            "Ayu::IsLocalChannelBroadcast(item)",
            "Ayu::RefreshLocalChannelBroadcastMetrics(item)",
        ),
        "Telegram/SourceFiles/data/data_message_reactions.cpp": (
            "Ayu::IsLocalChannelBroadcast(item)",
            "Ayu::RefreshLocalChannelBroadcastMetrics(item)",
            "LookupPossibleReactions(&item->history()->session())",
            "item->cancelScheduledPaidReaction();",
        ),
        "Telegram/SourceFiles/history/view/history_view_list_widget.cpp": (
            "Ayu::IncrementLocalChannelBroadcastReaction(",
        ),
        "Telegram/SourceFiles/history/history_inner_widget.cpp": (
            "AyuUi::AddLocalChannelBroadcastActions(",
            "Ayu::IncrementLocalChannelBroadcastReaction(",
            "AttachSelectorToMenu(",
        ),
        "Telegram/SourceFiles/history/view/reactions/history_view_reactions_selector.cpp": (
            "Ayu::IsLocalChannelBroadcast(item)",
        ),
        "Telegram/SourceFiles/ayu/ui/discreet_mode.cpp": (
            "Telegram Desktop",
            "IntelGram Desktop",
            "setApplicationDisplayName",
            "refreshApplicationIcon",
            "DestroyGlobalMenu",
            "CreateGlobalMenu",
        ),
        "Telegram/SourceFiles/window/window_main_menu.cpp": (
            "handleBrandingClick()",
            "kBrandingClickTimeout = 1200",
            "base::install_event_filter(_telegram",
            "QEvent::MouseButtonDblClick",
            "QEvent::MouseButtonRelease",
            "_brandingClickTimer.callOnce(kBrandingClickTimeout)",
            "Ayu::SetDiscreetMode(!Ayu::DiscreetModeEnabled())",
        ),
        "Telegram/SourceFiles/settings/sections/settings_main.cpp": (
            "Ayu::DiscreetModeEnabled()",
            "AyuMain::Id()",
        ),
        "Telegram/SourceFiles/ayu/ui/ayu_logo.cpp": (
            "Ayu::DiscreetModeEnabled()",
            "TELEGRAM_ICON",
        ),
        "Telegram/SourceFiles/ayu/ui/context_menu/context_menu.cpp": (
            'u"IntelGram"_q',
            "tr::ayu_LocalChannelOwnershipMenu(tr::now)",
            "Ayu::ShowLocalChannelOwnership(",
            "Ayu::IsLocalChannelBroadcast(item)",
            "void AddLocalChannelBroadcastActions(",
            "Ayu::ShowEditLocalChannelBroadcast(",
            "Ayu::ToggleLocalChannelBroadcastPinned(",
            "Ayu::ConfirmDeleteLocalChannelBroadcast(",
            "Ayu::LocalChannelBroadcastLink(",
        ),
        "Telegram/SourceFiles/history/history_widget.cpp": (
            "Ayu::HandleLocalChannelBroadcast(",
            "Ayu::EnsureLocalChannelBroadcasts(_history)",
            "Ayu::InterceptLocalChannelUnsupportedSend(",
            "Ayu::LocalChannelOwnershipActive(_peer)",
        ),
        "Telegram/SourceFiles/history/view/history_view_chat_section.cpp": (
            "Ayu::HandleLocalChannelBroadcast(",
            "Ayu::EnsureLocalChannelBroadcasts(_history)",
            "Ayu::InterceptLocalChannelUnsupportedSend(",
            "Ayu::LocalChannelOwnershipActive(_peer)",
        ),
        "Telegram/SourceFiles/info/profile/info_profile_top_bar.cpp": (
            "Ayu::LocalChannelOwnershipActive(peer)",
            "Ayu::ShowLocalChannelOwnership(",
        ),
        "Telegram/SourceFiles/window/window_peer_menu.cpp": (
            "Ayu::LocalChannelOwnershipActive(_peer)",
            "Ayu::ShowLocalChannelOwnership(",
        ),
        "Telegram/SourceFiles/ui/controls/silent_toggle.cpp": (
            "_localOnly(localOnly)",
            "if (!_localOnly)",
            "_channel->owner().notifySettings().update(",
        ),
        "Telegram/SourceFiles/info/profile/info_profile_actions.cpp": (
            "LocalProfileChannelShowcaseActive(user)",
            "ayu_LocalProfileChannelProfileLabel",
            "Ayu::ShowLocalChannelWorkspace(window);",
            "Ayu::LocalChannelDisplayTitle(channel)",
        ),
        "Telegram/SourceFiles/ayu/ui/utils/ayu_profile_values.cpp": (
            "LocalProfileBadgeStyleFor(",
            "LocalProfileEmojiStatusId(",
            "LocalProfileBotVerifyDetails(",
            "ResolveLocalProfileCloneUser(peer)",
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
        "Telegram/SourceFiles/ayu/ayu_settings.h": (
            "enum class LocalProfileBadgeStyle",
            "LocalProfileBadgeStyle::Automatic",
            "LocalProfileBadgeStyle::Premium",
            "LocalProfileBadgeStyle::Verified",
            "LocalProfileBadgeStyle::Scam",
            "LocalProfileBadgeStyle::Fake",
            "LocalProfileBadgeStyle::Direct",
            "localProfileBadgeStyleValue()",
            "localProfileEmojiStatusIdValue()",
            "localProfileBotVerificationIdValue()",
        ),
        "Telegram/SourceFiles/ayu/ayu_settings.cpp": (
            '"localProfileBadgeStyle"',
            '"localProfileEmojiStatusId"',
            '"localProfileBotVerificationId"',
            "setLocalProfileBadgeStyle(",
            "setLocalProfileEmojiStatusId(",
            "setLocalProfileBotVerificationId(",
        ),
        "Telegram/SourceFiles/info/profile/info_profile_values.cpp": (
            "BadgeValueFromLocalStyle(",
            "rpl::producer<> updates = rpl::merge(",
            "settings.localProfileBadgeStyleValue()",
            "Ayu::LocalProfileCloneUser(peer)",
            "Ayu::LocalProfileEmojiStatusId(peer)",
        ),
        "Telegram/SourceFiles/info/profile/info_profile_badge.cpp": (
            "settings.localProfileBotVerificationIdValue()",
            "Ayu::LocalProfileBotVerifyDetails(peer)",
            "BadgeType::BotVerified",
        ),
        "Telegram/SourceFiles/ui/unread_badge.cpp": (
            "Ayu::LocalProfileBadgeStyleFor(original)",
            "Ayu::LocalProfileCloneUser(original)",
            "Ayu::LocalProfileEmojiStatusId(original)",
            "LocalProfileBadgeStyle::Verified",
            "LocalProfileBadgeStyle::Premium",
        ),
        "Telegram/SourceFiles/dialogs/dialogs_inner_widget.cpp": (
            "Ayu::LocalProfileBotVerifyDetails(peer)",
        ),
        "Telegram/SourceFiles/history/view/history_view_top_bar_widget.cpp": (
            "Ayu::LocalProfileBotVerifyDetails(namePeer)",
        ),
        "Telegram/SourceFiles/info/profile/info_profile_inner_widget.cpp": (
            "Ayu::LocalProfileBotVerifyDetails(peer)",
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
            "AyuUi::AddLocalChannelBroadcastActions(",
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
        [
            "git",
            "-C",
            str(root),
            "diff",
            "--unified=0",
            "--no-ext-diff",
            BASELINE,
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
        "MTPmessages_SendPaidReaction",
        "MTPmessages_SendReaction",
        "MTPmessages_SendMessage",
        "MTPmessages_UpdatePinnedMessage",
        "MTPpayments_TransferStarGift",
        "MTPpayments_ToggleStarGiftsPinnedToTop",
    )
    for needle in forbidden:
        if needle in added:
            fail(f"unexpected Telegram mutation reference {needle}")
    local_ownership = (
        root
        / "Telegram/SourceFiles/ayu/ui/local_channel_ownership.cpp"
    ).read_text(encoding="utf-8", errors="strict")
    for needle in (
        ".api().request(",
        "MTPchannels_",
        "MTPmessages_Send",
        "MTPmessages_GetMessagesViews",
    ):
        if needle in local_ownership:
            fail(f"local ownership contains network mutation path {needle}")
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
