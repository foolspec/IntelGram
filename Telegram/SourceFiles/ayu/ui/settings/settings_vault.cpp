// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_vault.h"

#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "ayu/data/intelgram_export.h"
#include "ayu/data/intelgram_vault.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_ayu.h"
#include "ayu/ui/settings/settings_main.h"
#include "ayu/utils/telegram_helpers.h"
#include "base/unixtime.h"
#include "boxes/abstract_box.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "dialogs/dialogs_key.h"
#include "history/history_item.h"
#include "lang_auto.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/toast/toast.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/password_input.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_peer_menu.h"
#include "window/window_session_controller.h"

#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QDate>
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QtCore/QTime>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QFileDialog>

#include <algorithm>
#include <cstdlib>
#include <memory>

namespace Settings {
namespace {

using IntelGram::Vault::ID;
using namespace AyuBuilder;
using namespace Builder;

[[nodiscard]] ID CurrentAccount(not_null<Main::Session *> session) {
	return session->userId().bare & PeerId::kChatTypeMask;
}

[[nodiscard]] ID CurrentAccount(not_null<Window::SessionController *> controller) {
	return CurrentAccount(&controller->session());
}

[[nodiscard]] QString AccountLabel(ID accountId) {
	if (!accountId) {
		return tr::ayu_VaultAllAccounts(tr::now);
	}
	const auto session = getSession(accountId);
	return session ? session->user()->name() : QString::number(accountId);
}

[[nodiscard]] PeerData *ResolvePeer(const IntelGram::Vault::MessageRecord &entry) {
	const auto session = getSession(entry.accountId);
	if (!session) {
		return nullptr;
	}
	const auto bare = std::abs(entry.dialogId);
	if (entry.dialogId > 0) {
		return session->data().userLoaded(UserId(bare));
	}
	if (const auto channel = session->data().channelLoaded(ChannelId(bare))) {
		return channel;
	}
	return session->data().chatLoaded(ChatId(bare));
}

void OpenMessage(const IntelGram::Vault::MessageRecord &entry) {
	const auto peer = ResolvePeer(entry);
	if (!peer) {
		return;
	}
	const auto controller = peer->session().tryResolveWindow(peer);
	if (!controller) {
		return;
	}
	auto params = Window::SectionShow{Window::SectionShow::Way::Forward};
	params.allowDuplicateInStack = true;
	controller->showPeerHistory(peer, params, MsgId(entry.messageId));
}

void OpenRuleMatch(not_null<Window::SessionController *> controller,
				   const IntelGram::Vault::RuleMatchRecord &match) {
	const auto entry = IntelGram::Vault::MessageRecord{
		.accountId = match.accountId,
		.dialogId = match.dialogId,
		.messageId = match.messageId,
	};
	if (match.requiresConfirmation && !match.protectedReference) {
		if (const auto peer = ResolvePeer(entry)) {
			const auto fullId = FullMsgId(peer->id, MsgId(match.messageId));
			if (const auto item = peer->owner().message(fullId)) {
				if (item->allowsForward()) {
					Window::ShowForwardMessagesBox(controller, MessageIdsList{1, fullId});
					return;
				}
			}
		}
	}
	OpenMessage(entry);
}

[[nodiscard]] QString EntryTitle(const IntelGram::Vault::SearchResult &entry) {
	const auto date = QDateTime::fromSecsSinceEpoch(entry.date)
						  .toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
	return u"["_q + AccountLabel(entry.accountId) + u"] "_q + entry.dialogTitle + u" · "_q + date;
}

[[nodiscard]] QString EntryDescription(const IntelGram::Vault::SearchResult &entry) {
	if (entry.protectedContent) {
		return tr::ayu_VaultProtectedReference(tr::now);
	}
	auto result = entry.snippet;
	result.replace(u"<b>"_q, QString());
	result.replace(u"</b>"_q, QString());
	if (result.isEmpty()) {
		result = entry.mediaName.isEmpty() ? entry.mediaType : entry.mediaName;
	}
	return result.left(280);
}

void AddResultRows(not_null<Ui::VerticalLayout *> content,
				   const std::vector<IntelGram::Vault::SearchResult> &results) {
	if (results.empty()) {
		Ui::AddDividerText(content, tr::ayu_VaultNoResults());
		return;
	}
	for (const auto &entry : results) {
		const auto button = content->add(object_ptr<Ui::SettingsButton>(
			content, rpl::single(EntryTitle(entry)), st::settingsButtonNoIcon));
		button->setClickedCallback([entry] { OpenMessage(entry); });
		Ui::AddDividerText(content, rpl::single(EntryDescription(entry)));
	}
}

void ShowResults(not_null<Window::SessionController *> controller,
				 rpl::producer<QString> title,
				 std::vector<IntelGram::Vault::SearchResult> results) {
	controller->show(Box([=, results = std::move(results)](not_null<Ui::GenericBox *> box) mutable {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(std::move(title));
		AddResultRows(box->verticalLayout(), results);
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowVaultSearch(not_null<Window::SessionController *> controller, bool allAccounts) {
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(allAccounts ? tr::ayu_VaultUniversalSearch() : tr::ayu_VaultSearch());
		const auto field = box->addRow(object_ptr<Ui::InputField>(box, st::defaultInputField,
																  tr::ayu_VaultSearchPlaceholder()),
									   st::boxRowPadding);
		box->setFocusCallback([=] { field->setFocusFast(); });
		Ui::AddDividerText(box->verticalLayout(), tr::ayu_VaultSearchDescription());
		const auto search = [=] {
			const auto query = field->getLastText().trimmed();
			const auto results = IntelGram::Vault::Search(
				allAccounts ? 0 : CurrentAccount(controller), query, 100);
			box->closeBox();
			ShowResults(controller,
						allAccounts ? tr::ayu_VaultUniversalSearch() : tr::ayu_VaultSearch(),
						results);
		};
		field->submits() | rpl::on_next(search, field->lifetime());
		box->addButton(tr::ayu_VaultSearchAction(), search);
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void ShowSmartFolder(not_null<Window::SessionController *> controller,
					 IntelGram::Vault::SmartFolder folder,
					 rpl::producer<QString> title,
					 bool allAccounts = false) {
	ShowResults(controller, std::move(title),
				IntelGram::Vault::SmartFolderMessages(allAccounts ? 0 : CurrentAccount(controller),
													  folder, 100));
}

void ShowTimeline(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	const auto dialogId = peer ? getDialogIdFromPeer(peer) : 0;
	const auto stats = IntelGram::Vault::Stats(CurrentAccount(controller), dialogId);
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(tr::ayu_VaultTimeline());
		const auto content = box->verticalLayout();
		Ui::AddSubsectionTitle(content, peer ? rpl::single(peer->name()) : tr::ayu_VaultAllChats());
		Ui::AddDividerText(
			content, rpl::single(tr::ayu_VaultStatsMessages(tr::now, lt_count, stats.messages)));
		Ui::AddDividerText(content,
						   rpl::single(tr::ayu_VaultStatsMedia(tr::now, lt_count, stats.media)));
		Ui::AddDividerText(content,
						   rpl::single(tr::ayu_VaultStatsLinks(tr::now, lt_count, stats.links)));
		Ui::AddDividerText(content,
						   rpl::single(tr::ayu_VaultStatsEdits(tr::now, lt_count, stats.edits)));
		Ui::AddDividerText(
			content, rpl::single(tr::ayu_VaultStatsDeleted(tr::now, lt_count, stats.deleted)));
		Ui::AddDividerText(
			content,
			rpl::single(tr::ayu_VaultStatsProtected(tr::now, lt_count, stats.protectedReferences)));
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowJumpToDate(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	if (!peer) {
		controller->showToast(tr::ayu_VaultOpenChatFirst(tr::now));
		return;
	}
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(tr::ayu_VaultJumpToDate());
		const auto field = box->addRow(
			object_ptr<Ui::InputField>(box, st::defaultInputField, tr::ayu_VaultDatePlaceholder()),
			st::boxRowPadding);
		const auto jump = [=] {
			const auto date = QDate::fromString(field->getLastText().trimmed(), Qt::ISODate);
			if (!date.isValid()) {
				controller->showToast(tr::ayu_VaultDateInvalid(tr::now));
				return;
			}
			controller->session().api().resolveJumpToDate(
				Dialogs::Key(controller->session().data().history(peer)), date,
				[=](not_null<PeerData *> resolved, MsgId id) {
					box->closeBox();
					controller->showPeerHistory(resolved, Window::SectionShow::Way::Forward, id);
				});
		};
		field->submits() | rpl::on_next(jump, field->lifetime());
		box->addButton(tr::ayu_VaultJump(), jump);
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void ShowMediaHistory(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	const auto dialogId = peer ? getDialogIdFromPeer(peer) : 0;
	ShowResults(controller, tr::ayu_VaultMediaHistory(),
				IntelGram::Vault::MediaHistory(CurrentAccount(controller), dialogId, 200));
}

void ShowRevisionHistory(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	const auto dialogId = peer ? getDialogIdFromPeer(peer) : 0;
	const auto revisions
		= IntelGram::Vault::Revisions(CurrentAccount(controller), dialogId, 0, 500);
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_VaultRevisionHistory());
		const auto content = box->verticalLayout();
		if (revisions.empty()) {
			Ui::AddDividerText(content, tr::ayu_VaultRevisionHistoryEmpty());
		}
		for (const auto &revision : revisions) {
			const auto date = QDateTime::fromSecsSinceEpoch(revision.observedAt)
								  .toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
			Ui::AddDividerText(content,
							   rpl::single(date + u" · #"_q + QString::number(revision.messageId)
										   + u"\n"_q + revision.text));
		}
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowMoments(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	const auto dialogId = peer ? getDialogIdFromPeer(peer) : 0;
	auto records = IntelGram::Vault::Moments(CurrentAccount(controller), dialogId, 200);
	controller->show(Box([=, records = std::move(records)](not_null<Ui::GenericBox *> box) mutable {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_VaultMoments());
		const auto content = box->verticalLayout();
		if (records.empty()) {
			Ui::AddDividerText(content, tr::ayu_VaultMomentsEmpty());
		}
		for (const auto &moment : records) {
			const auto button = content->add(object_ptr<Ui::SettingsButton>(
				content,
				rpl::single(moment.title.isEmpty() ? tr::ayu_VaultMomentDefault(tr::now)
												   : moment.title),
				st::settingsButtonNoIcon));
			button->setClickedCallback([moment] {
				OpenMessage(IntelGram::Vault::MessageRecord{
					.accountId = moment.accountId,
					.dialogId = moment.dialogId,
					.messageId = moment.messageId,
				});
			});
			Ui::AddDividerText(
				content,
				rpl::single(moment.protectedReference ? tr::ayu_VaultProtectedReference(tr::now)
													  : moment.note));
		}
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowProfileNoteHistory(not_null<Window::SessionController *> controller,
							ID peerId,
							QString title) {
	const auto records
		= IntelGram::Vault::ProfileNoteHistory(CurrentAccount(controller), peerId, 100);
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(rpl::single(title));
		const auto content = box->verticalLayout();
		if (records.empty()) {
			Ui::AddDividerText(content, tr::ayu_VaultProfileNoteHistoryEmpty());
		}
		for (const auto &record : records) {
			const auto date = QDateTime::fromSecsSinceEpoch(record.updatedAt)
								  .toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
			Ui::AddSubsectionTitle(content, rpl::single(date));
			Ui::AddDividerText(
				content,
				rpl::single(record.relationship
							+ (record.tags.isEmpty() ? QString() : u"\n"_q + record.tags)
							+ (record.note.isEmpty() ? QString() : u"\n"_q + record.note)));
		}
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowProfileNote(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	if (!peer || peer->isSelf()) {
		controller->showToast(tr::ayu_VaultOpenContactFirst(tr::now));
		return;
	}
	const auto accountId = CurrentAccount(controller);
	const auto peerId = ID(peer->id.value & PeerId::kChatTypeMask);
	const auto existing = IntelGram::Vault::ProfileNote(accountId, peerId)
							  .value_or(IntelGram::Vault::ProfileNoteRecord{
								  .accountId = accountId,
								  .peerId = peerId,
							  });
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(rpl::single(peer->name()));
		const auto note
			= box->addRow(object_ptr<Ui::InputField>(box, st::defaultInputField,
													 tr::ayu_VaultNote(), existing.note),
						  st::boxRowPadding);
		const auto tags
			= box->addRow(object_ptr<Ui::InputField>(box, st::defaultInputField,
													 tr::ayu_VaultTags(), existing.tags),
						  st::boxRowPadding);
		const auto relationship = box->addRow(
			object_ptr<Ui::InputField>(box, st::defaultInputField, tr::ayu_VaultRelationship(),
									   existing.relationship),
			st::boxRowPadding);
		const auto reminderText = existing.reminderAt
			? QDateTime::fromSecsSinceEpoch(existing.reminderAt).date().toString(Qt::ISODate)
			: QString();
		const auto reminder
			= box->addRow(object_ptr<Ui::InputField>(box, st::defaultInputField,
													 tr::ayu_VaultReminderDate(), reminderText),
						  st::boxRowPadding);
		Ui::AddDividerText(box->verticalLayout(), tr::ayu_VaultNotesDescription());
		if (!IntelGram::Vault::ProfileNoteHistory(accountId, peerId, 1).empty()) {
			const auto history = box->verticalLayout()->add(object_ptr<Ui::SettingsButton>(
				box->verticalLayout(), tr::ayu_VaultProfileNoteHistory(),
				st::settingsButtonNoIcon));
			history->setClickedCallback([=] {
				box->closeBox();
				ShowProfileNoteHistory(controller, peerId, peer->name());
			});
		}
		box->addButton(tr::lng_settings_save(), [=] {
			const auto reminderValue = reminder->getLastText().trimmed();
			const auto reminderDate = QDate::fromString(reminderValue, Qt::ISODate);
			if (!reminderValue.isEmpty() && !reminderDate.isValid()) {
				controller->showToast(tr::ayu_VaultReminderInvalid(tr::now));
				return;
			}
			IntelGram::Vault::SaveProfileNote({
				.accountId = accountId,
				.peerId = peerId,
				.note = note->getLastText().trimmed(),
				.tags = tags->getLastText().trimmed(),
				.relationship = relationship->getLastText().trimmed(),
				.reminderAt = reminderDate.isValid()
					? int(QDateTime(reminderDate, QTime(9, 0)).toSecsSinceEpoch())
					: 0,
			});
			IntelGram::Vault::CaptureIdentity(peer);
			box->closeBox();
			controller->showToast(tr::ayu_VaultNoteSaved(tr::now));
		});
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void ShowProfileNotesList(not_null<Window::SessionController *> controller) {
	const auto records = IntelGram::Vault::ProfileNotes(CurrentAccount(controller));
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_VaultAllProfileNotes());
		const auto content = box->verticalLayout();
		if (records.empty()) {
			Ui::AddDividerText(content, tr::ayu_VaultProfileNotesEmpty());
		}
		for (const auto &record : records) {
			auto title = record.relationship.isEmpty() ? u"ID "_q + QString::number(record.peerId)
													   : record.relationship;
			if (record.reminderAt) {
				title += u" · "_q
					+ QDateTime::fromSecsSinceEpoch(record.reminderAt).date().toString(Qt::ISODate);
			}
			Ui::AddSubsectionTitle(content, rpl::single(std::move(title)));
			Ui::AddDividerText(
				content,
				rpl::single(record.note
							+ (record.tags.isEmpty() ? QString() : u"\n"_q + record.tags)));
		}
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowDueProfileNotes(not_null<Window::SessionController *> controller) {
	auto records = IntelGram::Vault::ProfileNotes(CurrentAccount(controller));
	const auto now = int(base::unixtime::now());
	records.erase(std::remove_if(records.begin(), records.end(),
								 [=](const auto &record) {
									 return !record.reminderAt || record.reminderAt > now;
								 }),
				  records.end());
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_VaultDueProfileRemindersTitle());
		const auto content = box->verticalLayout();
		for (const auto &record : records) {
			Ui::AddSubsectionTitle(content,
								   rpl::single(record.relationship.isEmpty()
												   ? u"ID "_q + QString::number(record.peerId)
												   : record.relationship));
			Ui::AddDividerText(content, rpl::single(record.note));
		}
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowIdentityInspector(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	if (!peer || peer->isSelf()) {
		controller->showToast(tr::ayu_VaultOpenContactFirst(tr::now));
		return;
	}
	IntelGram::Vault::CaptureIdentity(peer);
	const auto peerId = ID(peer->id.value & PeerId::kChatTypeMask);
	const auto history = IntelGram::Vault::IdentityHistory(CurrentAccount(controller), peerId, 100);
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_VaultIdentityInspector());
		const auto content = box->verticalLayout();
		Ui::AddSubsectionTitle(content, rpl::single(peer->name()));
		for (const auto &entry : history) {
			const auto date = QDateTime::fromSecsSinceEpoch(entry.observedAt)
								  .toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
			const auto shared = tr::ayu_VaultSharedGroups(tr::now, lt_count, entry.sharedGroups);
			Ui::AddDividerText(
				content,
				rpl::single(date + u"\n"_q + entry.displayName + u"\n"_q + entry.usernames + u"\n"_q
							+ entry.publicFlags + u"\n"_q + shared));
		}
		Ui::AddDividerText(content, tr::ayu_VaultIdentityDescription());
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowChatPolicy(not_null<Window::SessionController *> controller) {
	const auto peer = controller->activeChatCurrent().peer();
	if (!peer) {
		controller->showToast(tr::ayu_VaultOpenChatFirst(tr::now));
		return;
	}
	const auto accountId = CurrentAccount(controller);
	const auto dialogId = getDialogIdFromPeer(peer);
	const auto existing = IntelGram::Vault::GetChatPolicy(accountId, dialogId);
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		const auto state = std::make_shared<IntelGram::Vault::ChatPolicy>(existing);
		box->setTitle(rpl::single(peer->name()));
		const auto tags
			= box->addRow(object_ptr<Ui::InputField>(box, st::defaultInputField,
													 tr::ayu_VaultTags(), existing.tags),
						  st::boxRowPadding);
		const auto priority = box->addRow(
			object_ptr<Ui::InputField>(box, st::defaultInputField, tr::ayu_VaultPriority(),
									   QString::number(existing.priority)),
			st::boxRowPadding);
		const auto download = box->addRow(
			object_ptr<Ui::InputField>(box, st::defaultInputField, tr::ayu_VaultDownloadMode(),
									   QString::number(existing.downloadMode)),
			st::boxRowPadding);
		const auto addToggle
			= [&](rpl::producer<QString> title, bool initial, Fn<void(bool)> save) {
				  const auto button = box->verticalLayout()->add(object_ptr<Ui::SettingsButton>(
					  box->verticalLayout(), std::move(title), st::settingsButtonNoIcon));
				  button->toggleOn(rpl::single(initial));
				  button->toggledValue() | rpl::on_next(std::move(save), button->lifetime());
			  };
		addToggle(tr::ayu_VaultReadReminderValue(), existing.readReminder,
				  [=](bool value) { state->readReminder = value; });
		addToggle(tr::ayu_VaultPrivateDraftValue(), existing.privateDraft,
				  [=](bool value) { state->privateDraft = value; });
		addToggle(tr::ayu_VaultLocalMuteValue(), existing.localMute,
				  [=](bool value) { state->localMute = value; });
		Ui::AddDividerText(box->verticalLayout(), tr::ayu_VaultChatPolicyDescription());
		box->addButton(tr::lng_settings_save(), [=] {
			IntelGram::Vault::SaveChatPolicy({
				.accountId = accountId,
				.dialogId = dialogId,
				.tags = tags->getLastText().trimmed(),
				.priority = std::clamp(priority->getLastText().toInt(), 0, 3),
				.downloadMode = std::clamp(download->getLastText().toInt(), 0, 3),
				.readReminder = state->readReminder,
				.privateDraft = state->privateDraft,
				.localMute = state->localMute,
			});
			box->closeBox();
			controller->showToast(tr::ayu_VaultPolicySaved(tr::now));
		});
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void ShowRuleEditor(not_null<Window::SessionController *> controller,
					IntelGram::Vault::RuleMatch match,
					IntelGram::Vault::RuleAction action) {
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(tr::ayu_VaultRuleNew());
		const auto name = box->addRow(
			object_ptr<Ui::InputField>(box, st::defaultInputField, tr::ayu_VaultRuleName()),
			st::boxRowPadding);
		const auto pattern = box->addRow(
			object_ptr<Ui::InputField>(box, st::defaultInputField, tr::ayu_VaultRulePattern()),
			st::boxRowPadding);
		const auto value = box->addRow(
			object_ptr<Ui::InputField>(box, st::defaultInputField, tr::ayu_VaultRuleValue()),
			st::boxRowPadding);
		pattern->setVisible(match == IntelGram::Vault::RuleMatch::Keyword);
		Ui::AddDividerText(box->verticalLayout(),
						   (action == IntelGram::Vault::RuleAction::QueueForward)
							   ? tr::ayu_VaultRuleForwardDescription()
							   : tr::ayu_VaultRuleDescription());
		box->addButton(tr::ayu_VaultRuleSave(), [=] {
			const auto id = IntelGram::Vault::SaveRule({
				.accountId = CurrentAccount(controller),
				.name = name->getLastText().trimmed(),
				.match = match,
				.pattern = pattern->getLastText().trimmed(),
				.action = action,
				.actionValue = value->getLastText().trimmed(),
				.enabled = true,
			});
			if (!id) {
				controller->showToast(tr::ayu_VaultRuleInvalid(tr::now));
				return;
			}
			box->closeBox();
			controller->showToast(tr::ayu_VaultRuleSaved(tr::now));
		});
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void ShowRuleActions(not_null<Window::SessionController *> controller,
					 IntelGram::Vault::RuleMatch match) {
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(tr::ayu_VaultRuleAction());
		const auto content = box->verticalLayout();
		const auto add = [&](rpl::producer<QString> title, auto action) {
			const auto button = content->add(object_ptr<Ui::SettingsButton>(
				content, std::move(title), st::settingsButtonNoIcon));
			button->setClickedCallback([=] {
				box->closeBox();
				ShowRuleEditor(controller, match, action);
			});
		};
		add(tr::ayu_VaultRuleTag(), IntelGram::Vault::RuleAction::Tag);
		add(tr::ayu_VaultRuleSaveMoment(), IntelGram::Vault::RuleAction::Save);
		add(tr::ayu_VaultRuleNotify(), IntelGram::Vault::RuleAction::Notify);
		add(tr::ayu_VaultRuleMute(), IntelGram::Vault::RuleAction::MuteLocally);
		add(tr::ayu_VaultRuleQueueForward(), IntelGram::Vault::RuleAction::QueueForward);
		add(tr::ayu_VaultRuleSpam(), IntelGram::Vault::RuleAction::MarkSpam);
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void ShowRuleTypes(not_null<Window::SessionController *> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(tr::ayu_VaultRuleMatch());
		const auto content = box->verticalLayout();
		const auto add = [&](rpl::producer<QString> title, auto match) {
			const auto button = content->add(object_ptr<Ui::SettingsButton>(
				content, std::move(title), st::settingsButtonNoIcon));
			button->setClickedCallback([=] {
				box->closeBox();
				ShowRuleActions(controller, match);
			});
		};
		add(tr::ayu_VaultRuleKeyword(), IntelGram::Vault::RuleMatch::Keyword);
		add(tr::ayu_VaultRuleLink(), IntelGram::Vault::RuleMatch::Link);
		add(tr::ayu_VaultRulePhoto(), IntelGram::Vault::RuleMatch::Photo);
		add(tr::ayu_VaultRuleFile(), IntelGram::Vault::RuleMatch::File);
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void ShowRules(not_null<Window::SessionController *> controller) {
	const auto rules = IntelGram::Vault::Rules(CurrentAccount(controller));
	const auto matches = IntelGram::Vault::RuleMatches(CurrentAccount(controller), 100);
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setWidth(st::boxWideWidth);
		box->setNoContentMargin(true);
		box->setTitle(tr::ayu_VaultRules());
		const auto content = box->verticalLayout();
		Ui::AddSubsectionTitle(content, tr::ayu_VaultRulesConfigured());
		if (rules.empty()) {
			Ui::AddDividerText(content, tr::ayu_VaultRulesEmpty());
		}
		for (const auto &rule : rules) {
			const auto label
				= rule.name + (rule.pattern.isEmpty() ? QString() : u" · "_q + rule.pattern);
			const auto button = content->add(object_ptr<Ui::SettingsButton>(
				content, rpl::single(label), st::settingsButtonNoIcon));
			button->setClickedCallback([=] {
				controller->show(Ui::MakeConfirmBox({
					.text = tr::ayu_VaultRuleDeleteConfirm(),
					.confirmed =
						[=](Fn<void()> close) {
							if (!IntelGram::Vault::DeleteRule(rule.id)) {
								close();
								controller->showToast(tr::ayu_VaultRuleDeleteFailed(tr::now));
								return;
							}
							close();
							box->closeBox();
							controller->showToast(tr::ayu_VaultRuleDeleted(tr::now));
						},
					.confirmText = tr::lng_box_delete(),
					.cancelText = tr::lng_cancel(),
					.confirmStyle = &st::attentionBoxButton,
				}));
			});
			Ui::AddDividerText(
				content,
				tr::ayu_VaultRuleAccount(lt_item, rpl::single(AccountLabel(rule.accountId))));
		}
		Ui::AddSubsectionTitle(content, tr::ayu_VaultRuleActivity());
		if (matches.empty()) {
			Ui::AddDividerText(content, tr::ayu_VaultRuleActivityEmpty());
		}
		for (const auto &match : matches) {
			const auto button = content->add(object_ptr<Ui::SettingsButton>(
				content,
				rpl::single(match.action + u" · "_q + QString::number(match.dialogId) + u" / "_q
							+ QString::number(match.messageId)),
				st::settingsButtonNoIcon));
			button->setClickedCallback([=] { OpenRuleMatch(controller, match); });
			if (match.protectedReference) {
				Ui::AddDividerText(content, tr::ayu_VaultProtectedReference());
			}
		}
		box->addButton(tr::ayu_VaultRuleAdd(), [=] {
			box->closeBox();
			ShowRuleTypes(controller);
		});
		box->addButton(tr::lng_box_done(), [=] { box->closeBox(); });
	}));
}

void ShowExport(not_null<Window::SessionController *> controller, bool encrypted) {
	const auto peer = controller->activeChatCurrent().peer();
	const auto currentDialogId = peer ? getDialogIdFromPeer(peer) : 0;
	controller->show(Box([=](not_null<Ui::GenericBox *> box) {
		box->setTitle(encrypted ? tr::ayu_VaultFrozenBackup() : tr::ayu_VaultExportCenter());
		const auto passphrase = [&]() -> Ui::PasswordInput * {
			if (!encrypted) {
				return nullptr;
			}
			const auto content = box->verticalLayout();
			auto wrap = object_ptr<Ui::RpWidget>(content);
			wrap->resize(content->width(), st::settingLocalPasscodeInputField.heightMin);
			const auto field = Ui::CreateChild<Ui::PasswordInput>(
				wrap.data(),
				st::settingLocalPasscodeInputField,
				tr::ayu_VaultBackupPassphrase());
			wrap->geometryValue(
			) | rpl::on_next([=](const QRect &geometry) {
				field->moveToLeft((geometry.width() - field->width()) / 2, 0);
			}, wrap->lifetime());
			content->add(std::move(wrap));
			return field;
		}();
		Ui::AddDividerText(
			box->verticalLayout(),
			encrypted ? tr::ayu_VaultFrozenBackupDescription() : tr::ayu_VaultExportDescription());
		const auto run = [=](ID dialogId) {
			const auto directory
				= QFileDialog::getExistingDirectory(box, tr::ayu_VaultChooseFolder(tr::now));
			if (directory.isEmpty()) {
				return;
			}
			const auto result
				= IntelGram::Export::Create(CurrentAccount(controller), directory,
											IntelGram::Export::Options{
												.encrypted = encrypted,
												.dialogId = dialogId,
												.passphrase = passphrase
													? passphrase->getLastText()
													: QString(),
											});
			if (!result.ok) {
				controller->showToast(result.error);
				return;
			}
			box->closeBox();
			controller->showToast(tr::ayu_VaultExportComplete(tr::now));
			QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
		};
		if (!encrypted && currentDialogId) {
			box->addButton(tr::ayu_VaultExportCurrentChat(), [=] { run(currentDialogId); });
		}
		box->addButton(tr::ayu_VaultExportCurrentAccount(), [=] { run(0); });
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void BuildSearchAndTimeline(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addSubsectionTitle(tr::ayu_VaultSearchAndTimeline());
	builder.addButton({
		.id = u"intelgram/vault/search"_q,
		.title = tr::ayu_VaultSearch(),
		.icon = {&st::menuIconSearch},
		.onClick = [=] { ShowVaultSearch(controller, false); },
	});
	builder.addButton({
		.id = u"intelgram/vault/universalSearch"_q,
		.title = tr::ayu_VaultUniversalSearch(),
		.icon = {&st::menuIconSearch},
		.onClick = [=] { ShowVaultSearch(controller, true); },
	});
	builder.addButton({
		.id = u"intelgram/vault/unifiedInbox"_q,
		.title = tr::ayu_VaultUnifiedInbox(),
		.icon = {&st::menuIconChats},
		.onClick =
			[=] {
				ShowSmartFolder(controller, IntelGram::Vault::SmartFolder::All,
								tr::ayu_VaultUnifiedInbox(), true);
			},
	});
	builder.addButton({
		.id = u"intelgram/vault/timeline"_q,
		.title = tr::ayu_VaultTimeline(),
		.icon = {&st::menuIconSchedule},
		.onClick = [=] { ShowTimeline(controller); },
	});
	builder.addButton({
		.id = u"intelgram/vault/jumpDate"_q,
		.title = tr::ayu_VaultJumpToDate(),
		.icon = {&st::menuIconSchedule},
		.onClick = [=] { ShowJumpToDate(controller); },
	});
	builder.addButton({
		.id = u"intelgram/vault/mediaHistory"_q,
		.title = tr::ayu_VaultMediaHistory(),
		.icon = {&st::menuIconShowAll},
		.onClick = [=] { ShowMediaHistory(controller); },
	});
	builder.addButton({
		.id = u"intelgram/vault/revisions"_q,
		.title = tr::ayu_VaultRevisionHistory(),
		.icon = {&st::menuIconArchive},
		.onClick = [=] { ShowRevisionHistory(controller); },
	});
	builder.addButton({
		.id = u"intelgram/vault/moments"_q,
		.title = tr::ayu_VaultMoments(),
		.icon = {&st::menuIconFave},
		.onClick = [=] { ShowMoments(controller); },
	});
}

void BuildSmartFolders(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addSubsectionTitle(tr::ayu_VaultSmartFolders());
	const auto add = [&](QString id, QString title, auto folder) {
		builder.addButton({
			.id = std::move(id),
			.title = rpl::single(title),
			.icon = {&st::menuIconShowInFolder},
			.onClick = [=] { ShowSmartFolder(controller, folder, rpl::single(title)); },
		});
	};
	add(u"intelgram/vault/unreadPeople"_q, tr::ayu_VaultFolderUnreadPeople(tr::now),
		IntelGram::Vault::SmartFolder::UnreadPeople);
	add(u"intelgram/vault/work"_q, tr::ayu_VaultFolderWork(tr::now),
		IntelGram::Vault::SmartFolder::Work);
	add(u"intelgram/vault/highPriority"_q, tr::ayu_VaultFolderHighPriority(tr::now),
		IntelGram::Vault::SmartFolder::HighPriority);
	add(u"intelgram/vault/needsReply"_q, tr::ayu_VaultFolderNeedsReply(tr::now),
		IntelGram::Vault::SmartFolder::NeedsReply);
	add(u"intelgram/vault/spamReview"_q, tr::ayu_VaultFolderSpamReview(tr::now),
		IntelGram::Vault::SmartFolder::SpamReview);
	builder.addButton({
		.id = u"intelgram/vault/chatPolicy"_q,
		.title = tr::ayu_VaultChatPolicy(),
		.icon = {&st::menuIconManage},
		.onClick = [=] { ShowChatPolicy(controller); },
	});
}

void BuildIdentityAndRules(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto notes = IntelGram::Vault::ProfileNotes(CurrentAccount(builder.session()));
	const auto now = int(base::unixtime::now());
	const auto due = std::count_if(notes.begin(), notes.end(), [=](const auto &record) {
		return record.reminderAt && record.reminderAt <= now;
	});
	builder.addSubsectionTitle(tr::ayu_VaultIdentityAndRules());
	if (due) {
		builder.addButton({
			.id = u"intelgram/vault/dueProfileNotes"_q,
			.title = tr::ayu_VaultDueProfileReminders(lt_count, rpl::single(1. * due)),
			.icon = {&st::menuIconNotifications},
			.onClick = [=] { ShowDueProfileNotes(controller); },
		});
	}
	builder.addButton({
		.id = u"intelgram/vault/profileNote"_q,
		.title = tr::ayu_VaultProfileNotes(),
		.icon = {&st::menuIconEdit},
		.onClick = [=] { ShowProfileNote(controller); },
	});
	builder.addButton({
		.id = u"intelgram/vault/allProfileNotes"_q,
		.title = tr::ayu_VaultAllProfileNotes(),
		.icon = {&st::menuIconShowAll},
		.onClick = [=] { ShowProfileNotesList(controller); },
	});
	builder.addButton({
		.id = u"intelgram/vault/identity"_q,
		.title = tr::ayu_VaultIdentityInspector(),
		.icon = {&st::menuIconProfile},
		.onClick = [=] { ShowIdentityInspector(controller); },
	});
	builder.addButton({
		.id = u"intelgram/vault/rules"_q,
		.title = tr::ayu_VaultRules(),
		.icon = {&st::menuIconBot},
		.onClick = [=] { ShowRules(controller); },
	});
}

void BuildPrivacy(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSubsectionTitle(tr::ayu_VaultPrivacyAndSpam());
	const auto option = [&](QString id, rpl::producer<QString> title, QString key) {
		ayu.addToggle({
			.id = std::move(id),
			.title = std::move(title),
			.getter = [key] { return IntelGram::Vault::Enabled(key, true); },
			.setter = [key](bool enabled) { IntelGram::Vault::SetEnabled(key, enabled); },
		});
	};
	option(u"intelgram/vault/enabled"_q, tr::ayu_VaultEnableIndex(), u"vault_enabled"_q);
	option(u"intelgram/vault/privateDrafts"_q, tr::ayu_VaultPrivateDrafts(), u"private_drafts"_q);
	option(u"intelgram/vault/readReminders"_q, tr::ayu_VaultReadReminders(), u"read_reminders"_q);
	option(u"intelgram/vault/inviteWarnings"_q, tr::ayu_VaultInviteWarnings(),
		   u"invite_warnings"_q);
	option(u"intelgram/vault/unknownReview"_q, tr::ayu_VaultUnknownReview(), u"unknown_review"_q);
	builder.addSectionButton({
		.title = tr::ayu_VaultGhostControls(),
		.targetSection = AyuGhost::Id(),
		.icon = {&st::menuIconStealth},
	});
}

void BuildExport(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addSubsectionTitle(tr::ayu_ExportArchiveHeader());
	builder.addButton({
		.id = u"intelgram/vault/export"_q,
		.title = tr::ayu_VaultExportCenter(),
		.icon = {&st::menuIconExport},
		.onClick = [=] { ShowExport(controller, false); },
	});
	builder.addButton({
		.id = u"intelgram/vault/frozenBackup"_q,
		.title = tr::ayu_VaultFrozenBackup(),
		.icon = {&st::menuIconLock},
		.onClick = [=] { ShowExport(controller, true); },
	});
	builder.addDividerText(tr::ayu_VaultProtectedPolicy());
}

const auto kMeta = BuildHelper(
	{
		.id = AyuVault::Id(),
		.parentId = AyuMain::Id(),
		.title = &tr::ayu_VaultTitle,
		.icon = &st::menuIconArchive,
	},
	[](SectionBuilder &builder) {
		builder.addSkip();
		BuildSearchAndTimeline(builder);
		AyuSectionBuilder(builder).addSectionDivider();
		BuildSmartFolders(builder);
		builder.addSkip();
	});

const auto kAutomationMeta = BuildHelper(
	{
		.id = AyuAutomation::Id(),
		.parentId = AyuMain::Id(),
		.title = &tr::ayu_AutomationTitle,
		.icon = &st::menuIconBot,
	},
	[](SectionBuilder &builder) {
		auto ayu = AyuSectionBuilder(builder);
		builder.addSkip();
		BuildIdentityAndRules(builder);
		AyuSectionBuilder(builder).addSectionDivider();
		BuildPrivacy(builder, ayu);
		builder.addSkip();
	});

const auto kExportMeta = BuildHelper(
	{
		.id = AyuExport::Id(),
		.parentId = AyuMain::Id(),
		.title = &tr::ayu_ExportTitle,
		.icon = &st::menuIconExport,
	},
	[](SectionBuilder &builder) {
		builder.addSkip();
		BuildExport(builder);
		builder.addSkip();
	});

} // namespace

rpl::producer<QString> AyuVault::title() {
	return tr::ayu_VaultTitle();
}

AyuVault::AyuVault(QWidget *parent, not_null<Window::SessionController *> controller)
	: Section(parent, controller) {
	setupContent();
}

void AyuVault::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

AyuAutomation::AyuAutomation(
	QWidget *parent,
	not_null<Window::SessionController *> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> AyuAutomation::title() {
	return tr::ayu_AutomationTitle();
}

void AyuAutomation::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kAutomationMeta.build);
	Ui::ResizeFitChild(this, content);
}

AyuExport::AyuExport(
	QWidget *parent,
	not_null<Window::SessionController *> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> AyuExport::title() {
	return tr::ayu_ExportTitle();
}

void AyuExport::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kExportMeta.build);
	Ui::ResizeFitChild(this, content);
}

} // namespace Settings
