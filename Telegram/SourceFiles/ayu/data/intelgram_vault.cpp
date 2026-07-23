// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/data/intelgram_vault.h"

#include "ayu/libs/sqlite/sqlite3.h"
#include "ayu/utils/telegram_helpers.h"
#include "base/unixtime.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_document.h"
#include "data/data_media_types.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_thread.h"
#include "data/data_user.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "main/main_session.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>

#include <mutex>

namespace IntelGram::Vault {
namespace {

constexpr auto kDatabasePath = "./tdata/intelgram_vault.db";

class Statement final {
public:
	Statement(sqlite3 *database, const char *sql) {
		if (sqlite3_prepare_v2(database, sql, -1, &_value, nullptr) != SQLITE_OK) {
			_value = nullptr;
		}
	}

	~Statement() {
		if (_value) {
			sqlite3_finalize(_value);
		}
	}

	[[nodiscard]] sqlite3_stmt *get() const { return _value; }

	[[nodiscard]] explicit operator bool() const { return _value != nullptr; }

private:
	sqlite3_stmt *_value = nullptr;

};

std::recursive_mutex DatabaseMutex;
sqlite3 *Database = nullptr;
bool FtsAvailable = false;

[[nodiscard]] bool Execute(const char *sql, bool logFailure = true) {
	char *error = nullptr;
	const auto result = sqlite3_exec(Database, sql, nullptr, nullptr, &error);
	if (result == SQLITE_OK) {
		return true;
	}
	if (logFailure) {
		LOG(("IntelGram Vault SQL error: %1").arg(QString::fromUtf8(error ? error : "unknown")));
	}
	if (error) {
		sqlite3_free(error);
	}
	return false;
}

void Bind(sqlite3_stmt *statement, int index, ID value) {
	sqlite3_bind_int64(statement, index, value);
}

void Bind(sqlite3_stmt *statement, int index, int value) {
	sqlite3_bind_int(statement, index, value);
}

void Bind(sqlite3_stmt *statement, int index, bool value) {
	sqlite3_bind_int(statement, index, value ? 1 : 0);
}

void Bind(sqlite3_stmt *statement, int index, const QString &value) {
	const auto utf8 = value.toUtf8();
	sqlite3_bind_text(statement, index, utf8.constData(), utf8.size(), SQLITE_TRANSIENT);
}

[[nodiscard]] QString ColumnText(sqlite3_stmt *statement, int index) {
	const auto value = sqlite3_column_text(statement, index);
	return value ? QString::fromUtf8(reinterpret_cast<const char *>(value)) : QString();
}

[[nodiscard]] ID AccountId(not_null<Main::Session *> session) {
	return session->userId().bare & PeerId::kChatTypeMask;
}

[[nodiscard]] ID AccountId(not_null<HistoryItem *> item) {
	return AccountId(&item->history()->owner().session());
}

[[nodiscard]] bool IsProtected(not_null<HistoryItem *> item) {
	const auto media = item->media();
	return item->forbidsSaving() || !item->history()->peer->allowsForwarding()
		|| item->ttlDestroyAt() || item->unsupportedTTL() || (media && media->ttlSeconds());
}

[[nodiscard]] QString DialogType(not_null<PeerData *> peer) {
	if (const auto user = peer->asUser()) {
		return user->isBot() ? u"bot"_q : u"person"_q;
	} else if (const auto channel = peer->asChannel()) {
		return channel->isMegagroup() ? u"group"_q : u"channel"_q;
	}
	return peer->asChat() ? u"group"_q : u"other"_q;
}

struct MediaInfo {
	QString type;
	QString name;
	QString mime;
	QString path;
};

[[nodiscard]] MediaInfo ReadMedia(not_null<HistoryItem *> item) {
	const auto media = item->media();
	if (!media) {
		return {};
	} else if (media->photo()) {
		return {u"photo"_q, QString(), u"image"_q};
	} else if (const auto document = media->document()) {
		auto type = u"file"_q;
		switch (document->type) {
			case VideoDocument:
				type = u"video"_q;
				break;
			case SongDocument:
				type = u"audio"_q;
				break;
			case StickerDocument:
				type = u"sticker"_q;
				break;
			case AnimatedDocument:
				type = u"animation"_q;
				break;
			case VoiceDocument:
				type = u"voice"_q;
				break;
			case RoundVideoDocument:
				type = u"video_message"_q;
				break;
			case WallPaperDocument:
				type = u"wallpaper"_q;
				break;
			case FileDocument:
				break;
		}
		return {
			type,
			document->filename(),
			document->mimeString(),
			document->filepath(true),
		};
	} else if (media->webpage()) {
		return {u"link"_q, QString(), u"text/html"_q, QString()};
	} else if (media->sharedContact()) {
		return {u"contact"_q, QString(), QString(), QString()};
	} else if (media->location()) {
		return {u"location"_q, QString(), QString(), QString()};
	} else if (media->poll()) {
		return {u"poll"_q, QString(), QString(), QString()};
	} else if (media->call()) {
		return {u"call"_q, QString(), QString(), QString()};
	}
	return {u"other"_q, QString(), QString()};
}

[[nodiscard]] QString ExtractLinks(const QString &text) {
	static const auto expression
		= QRegularExpression(uR"((?:(?:https?://|tg://|t\.me/)[^\s<>()]+))"_q,
							 QRegularExpression::CaseInsensitiveOption);
	auto result = QStringList();
	auto match = expression.globalMatch(text);
	while (match.hasNext()) {
		result.push_back(match.next().captured());
	}
	result.removeDuplicates();
	return result.join(u' ');
}

[[nodiscard]] QString FtsQuery(const QString &query) {
	static const auto terms = QRegularExpression(u"[\\p{L}\\p{N}_@#]+"_q);
	auto result = QStringList();
	auto match = terms.globalMatch(query);
	while (match.hasNext()) {
		auto term = match.next().captured();
		term.replace(u'"', u"\"\""_q);
		result.push_back(u'"' + term + u'"' + u'*');
	}
	return result.join(u" AND "_q);
}

void CreateSchema() {
	Execute("PRAGMA journal_mode=WAL;");
	Execute("PRAGMA synchronous=NORMAL;");
	Execute("PRAGMA secure_delete=ON;");
	Execute("PRAGMA foreign_keys=ON;");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_messages ("
		"account_id INTEGER NOT NULL, dialog_id INTEGER NOT NULL, "
		"message_id INTEGER NOT NULL, date INTEGER NOT NULL DEFAULT 0, "
		"edit_date INTEGER NOT NULL DEFAULT 0, sender_id INTEGER NOT NULL "
		"DEFAULT 0, "
		"topic_id INTEGER NOT NULL DEFAULT 0, dialog_title TEXT NOT NULL DEFAULT "
		"'', "
		"dialog_type TEXT NOT NULL DEFAULT '', text TEXT NOT NULL DEFAULT '', "
		"media_type TEXT NOT NULL DEFAULT '', media_name TEXT NOT NULL DEFAULT "
		"'', "
		"mime_type TEXT NOT NULL DEFAULT '', media_path TEXT NOT NULL DEFAULT '', "
		"links TEXT NOT NULL DEFAULT '', "
		"tags TEXT NOT NULL DEFAULT '', outgoing INTEGER NOT NULL DEFAULT 0, "
		"unread INTEGER NOT NULL DEFAULT 0, deleted INTEGER NOT NULL DEFAULT 0, "
		"protected INTEGER NOT NULL DEFAULT 0, updated_at INTEGER NOT NULL "
		"DEFAULT 0, "
		"UNIQUE(account_id, dialog_id, message_id));");
	Execute("ALTER TABLE vault_messages ADD COLUMN media_path TEXT NOT NULL DEFAULT '';", false);
	Execute(
		"CREATE INDEX IF NOT EXISTS vault_messages_account_date "
		"ON vault_messages(account_id, date DESC);");
	Execute(
		"CREATE INDEX IF NOT EXISTS vault_messages_dialog_date "
		"ON vault_messages(account_id, dialog_id, date DESC);");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_moments ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, account_id INTEGER NOT NULL, "
		"dialog_id INTEGER NOT NULL, message_id INTEGER NOT NULL, "
		"message_date INTEGER NOT NULL DEFAULT 0, created_at INTEGER NOT NULL, "
		"title TEXT NOT NULL DEFAULT '', note TEXT NOT NULL DEFAULT '', "
		"tags TEXT NOT NULL DEFAULT '', protected_reference INTEGER NOT NULL "
		"DEFAULT 0);");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_profile_notes ("
		"account_id INTEGER NOT NULL, peer_id INTEGER NOT NULL, "
		"note TEXT NOT NULL DEFAULT '', tags TEXT NOT NULL DEFAULT '', "
		"relationship TEXT NOT NULL DEFAULT '', reminder_at INTEGER NOT NULL "
		"DEFAULT 0, "
		"updated_at INTEGER NOT NULL, UNIQUE(account_id, peer_id));");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_profile_note_history ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, account_id INTEGER NOT NULL, "
		"peer_id INTEGER NOT NULL, note TEXT NOT NULL DEFAULT '', "
		"tags TEXT NOT NULL DEFAULT '', relationship TEXT NOT NULL DEFAULT '', "
		"reminder_at INTEGER NOT NULL DEFAULT 0, updated_at INTEGER NOT NULL);");
	Execute(
		"CREATE INDEX IF NOT EXISTS vault_profile_note_history_peer "
		"ON vault_profile_note_history(account_id,peer_id,updated_at DESC);");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_identity_snapshots ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, account_id INTEGER NOT NULL, "
		"peer_id INTEGER NOT NULL, observed_at INTEGER NOT NULL, "
		"display_name TEXT NOT NULL DEFAULT '', usernames TEXT NOT NULL "
		"DEFAULT '', "
		"public_flags TEXT NOT NULL DEFAULT '', shared_groups INTEGER NOT "
		"NULL DEFAULT 0);");
	Execute(
		"CREATE INDEX IF NOT EXISTS vault_identity_peer_date "
		"ON vault_identity_snapshots(account_id, peer_id, observed_at DESC);");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_rules ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, account_id INTEGER NOT NULL DEFAULT 0, "
		"name TEXT NOT NULL DEFAULT '', "
		"match_type INTEGER NOT NULL, pattern TEXT NOT NULL DEFAULT '', "
		"action INTEGER NOT NULL, action_value TEXT NOT NULL DEFAULT '', "
		"enabled INTEGER NOT NULL DEFAULT 1, created_at INTEGER NOT NULL);");
	Execute("ALTER TABLE vault_rules ADD COLUMN account_id INTEGER NOT NULL DEFAULT 0;", false);
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_revisions ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, account_id INTEGER NOT NULL, "
		"dialog_id INTEGER NOT NULL, message_id INTEGER NOT NULL, "
		"observed_at INTEGER NOT NULL, text TEXT NOT NULL DEFAULT '', "
		"UNIQUE(account_id,dialog_id,message_id,observed_at,text));");
	Execute(
		"CREATE INDEX IF NOT EXISTS vault_revision_message "
		"ON vault_revisions(account_id,dialog_id,message_id,observed_at DESC);");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_rule_matches ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT, rule_id INTEGER NOT NULL, "
		"account_id INTEGER NOT NULL, dialog_id INTEGER NOT NULL, "
		"message_id INTEGER NOT NULL, matched_at INTEGER NOT NULL, "
		"action TEXT NOT NULL DEFAULT '', requires_confirmation INTEGER NOT "
		"NULL DEFAULT 0, "
		"protected_reference INTEGER NOT NULL DEFAULT 0);");
	Execute(
		"CREATE UNIQUE INDEX IF NOT EXISTS vault_rule_match_message "
		"ON vault_rule_matches(rule_id,account_id,dialog_id,message_id);");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_chat_policies ("
		"account_id INTEGER NOT NULL, dialog_id INTEGER NOT NULL, "
		"tags TEXT NOT NULL DEFAULT '', priority INTEGER NOT NULL DEFAULT 0, "
		"download_mode INTEGER NOT NULL DEFAULT 0, read_reminder INTEGER NOT "
		"NULL DEFAULT 0, "
		"private_draft INTEGER NOT NULL DEFAULT 0, local_mute INTEGER NOT "
		"NULL DEFAULT 0, "
		"UNIQUE(account_id, dialog_id));");
	Execute(
		"CREATE TABLE IF NOT EXISTS vault_options ("
		"key TEXT PRIMARY KEY, value TEXT NOT NULL DEFAULT '');");

	FtsAvailable = Execute(
		"CREATE VIRTUAL TABLE IF NOT EXISTS vault_messages_fts USING fts5("
		"text, media_name, links, content='vault_messages', "
		"content_rowid='rowid', "
		"tokenize='unicode61 remove_diacritics 2');",
		false);
	if (FtsAvailable) {
		Execute(
			"CREATE TRIGGER IF NOT EXISTS vault_messages_ai AFTER INSERT ON "
			"vault_messages BEGIN "
			"INSERT INTO vault_messages_fts(rowid,text,media_name,links) "
			"VALUES(new.rowid,new.text,new.media_name,new.links); END;");
		Execute(
			"CREATE TRIGGER IF NOT EXISTS vault_messages_ad AFTER DELETE ON "
			"vault_messages BEGIN "
			"INSERT INTO "
			"vault_messages_fts(vault_messages_fts,rowid,text,media_name,links) "
			"VALUES('delete',old.rowid,old.text,old.media_name,old.links); END;");
		Execute(
			"CREATE TRIGGER IF NOT EXISTS vault_messages_au AFTER UPDATE ON "
			"vault_messages BEGIN "
			"INSERT INTO "
			"vault_messages_fts(vault_messages_fts,rowid,text,media_name,links) "
			"VALUES('delete',old.rowid,old.text,old.media_name,old.links); "
			"INSERT INTO vault_messages_fts(rowid,text,media_name,links) "
			"VALUES(new.rowid,new.text,new.media_name,new.links); END;");
		Execute(
			"INSERT INTO vault_messages_fts(vault_messages_fts) "
			"VALUES('rebuild');");
	}
}

void EnsureInitialized() {
	if (Database) {
		return;
	}
	QDir().mkpath(u"./tdata"_q);
	if (sqlite3_open_v2(kDatabasePath, &Database,
						SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr)
		!= SQLITE_OK) {
		LOG(("IntelGram Vault failed to open local database."));
		if (Database) {
			sqlite3_close(Database);
			Database = nullptr;
		}
		return;
	}
	sqlite3_busy_timeout(Database, 5000);
	CreateSchema();
}

[[nodiscard]] MessageRecord ReadMessage(sqlite3_stmt *statement) {
	auto result = MessageRecord();
	result.accountId = sqlite3_column_int64(statement, 0);
	result.dialogId = sqlite3_column_int64(statement, 1);
	result.messageId = sqlite3_column_int(statement, 2);
	result.date = sqlite3_column_int(statement, 3);
	result.editDate = sqlite3_column_int(statement, 4);
	result.senderId = sqlite3_column_int64(statement, 5);
	result.topicId = sqlite3_column_int64(statement, 6);
	result.dialogTitle = ColumnText(statement, 7);
	result.dialogType = ColumnText(statement, 8);
	result.text = ColumnText(statement, 9);
	result.mediaType = ColumnText(statement, 10);
	result.mediaName = ColumnText(statement, 11);
	result.mimeType = ColumnText(statement, 12);
	result.mediaPath = ColumnText(statement, 13);
	result.links = ColumnText(statement, 14);
	result.tags = ColumnText(statement, 15);
	result.outgoing = sqlite3_column_int(statement, 16) != 0;
	result.unread = sqlite3_column_int(statement, 17) != 0;
	result.deleted = sqlite3_column_int(statement, 18) != 0;
	result.protectedContent = sqlite3_column_int(statement, 19) != 0;
	return result;
}

const auto kSelectColumns
	= u"m.account_id,m.dialog_id,m.message_id,m.date,m.edit_date,m.sender_id,"
	  u"m.topic_id,m.dialog_title,m.dialog_type,m.text,m.media_type,m.media_name,"
	  u"m.mime_type,m.media_path,m.links,m.tags,m.outgoing,m.unread,m.deleted,m.protected"_q;

[[nodiscard]] QString RuleActionName(RuleAction action) {
	switch (action) {
		case RuleAction::Tag:
			return u"tag"_q;
		case RuleAction::Save:
			return u"save"_q;
		case RuleAction::Notify:
			return u"notify"_q;
		case RuleAction::MuteLocally:
			return u"mute"_q;
		case RuleAction::QueueForward:
			return u"queue_forward"_q;
		case RuleAction::MarkSpam:
			return u"spam"_q;
	}
	Unexpected("RuleAction in IntelGram Vault.");
}

[[nodiscard]] bool RuleMatches(const RuleRecord &rule, const MessageRecord &message) {
	switch (rule.match) {
		case RuleMatch::Keyword:
			return message.text.contains(rule.pattern, Qt::CaseInsensitive);
		case RuleMatch::Link:
			return !message.links.isEmpty();
		case RuleMatch::Photo:
			return message.mediaType == u"photo"_q;
		case RuleMatch::File:
			return !message.mediaType.isEmpty() && message.mediaType != u"photo"_q
				&& message.mediaType != u"link"_q;
	}
	Unexpected("RuleMatch in IntelGram Vault.");
}

void ApplyRules(const MessageRecord &message) {
	if (message.outgoing) {
		return;
	}
	for (const auto &rule : Rules(message.accountId)) {
		if (!rule.enabled || !RuleMatches(rule, message)) {
			continue;
		}
		const auto protectedForward
			= message.protectedContent && (rule.action == RuleAction::QueueForward);
		const auto action = protectedForward ? u"blocked_protected"_q : RuleActionName(rule.action);
		const auto needsConfirmation
			= !protectedForward && (rule.action == RuleAction::QueueForward);
		Statement insert(Database,
						 "INSERT OR IGNORE INTO "
						 "vault_rule_matches(rule_id,account_id,dialog_id,message_id,"
						 "matched_at,action,requires_confirmation,protected_reference) "
						 "VALUES(?,?,?,?,?,?,?,?);");
		auto inserted = false;
		if (insert) {
			Bind(insert.get(), 1, rule.id);
			Bind(insert.get(), 2, message.accountId);
			Bind(insert.get(), 3, message.dialogId);
			Bind(insert.get(), 4, message.messageId);
			Bind(insert.get(), 5, int(base::unixtime::now()));
			Bind(insert.get(), 6, action);
			Bind(insert.get(), 7, needsConfirmation);
			Bind(insert.get(), 8, message.protectedContent);
			inserted
				= (sqlite3_step(insert.get()) == SQLITE_DONE) && (sqlite3_changes(Database) != 0);
		}
		if (!inserted) {
			continue;
		}
		if (rule.action == RuleAction::Tag && !message.protectedContent) {
			Statement update(Database,
							 "UPDATE vault_messages SET tags=trim(tags || ' ' || ?) "
							 "WHERE account_id=? AND dialog_id=? AND message_id=?;");
			if (update) {
				Bind(update.get(), 1, rule.actionValue);
				Bind(update.get(), 2, message.accountId);
				Bind(update.get(), 3, message.dialogId);
				Bind(update.get(), 4, message.messageId);
				sqlite3_step(update.get());
			}
		} else if (rule.action == RuleAction::Save) {
			Statement moment(Database,
							 "INSERT INTO "
							 "vault_moments(account_id,dialog_id,message_id,message_date,"
							 "created_at,title,note,tags,protected_reference) "
							 "VALUES(?,?,?,?,?,?,?,?,?);");
			if (moment) {
				Bind(moment.get(), 1, message.accountId);
				Bind(moment.get(), 2, message.dialogId);
				Bind(moment.get(), 3, message.messageId);
				Bind(moment.get(), 4, message.date);
				Bind(moment.get(), 5, int(base::unixtime::now()));
				Bind(moment.get(), 6, rule.name);
				Bind(moment.get(), 7, QString());
				Bind(moment.get(), 8, rule.actionValue);
				Bind(moment.get(), 9, message.protectedContent);
				sqlite3_step(moment.get());
			}
		} else if (rule.action == RuleAction::MarkSpam) {
			Statement update(Database,
							 "UPDATE vault_messages SET tags=trim(tags || ' spam') "
							 "WHERE account_id=? AND dialog_id=? AND message_id=?;");
			if (update) {
				Bind(update.get(), 1, message.accountId);
				Bind(update.get(), 2, message.dialogId);
				Bind(update.get(), 3, message.messageId);
				sqlite3_step(update.get());
			}
		} else if (rule.action == RuleAction::MuteLocally) {
			Statement policy(Database,
							 "INSERT INTO vault_chat_policies(account_id,dialog_id,local_mute) "
							 "VALUES(?,?,1) ON CONFLICT(account_id,dialog_id) DO UPDATE SET "
							 "local_mute=1;");
			if (policy) {
				Bind(policy.get(), 1, message.accountId);
				Bind(policy.get(), 2, message.dialogId);
				sqlite3_step(policy.get());
			}
		}
	}
}

[[nodiscard]] QJsonObject MessageJson(const MessageRecord &message) {
	auto result = QJsonObject{
		{u"account_id"_q, QString::number(message.accountId)},
		{u"dialog_id"_q, QString::number(message.dialogId)},
		{u"message_id"_q, message.messageId},
		{u"date"_q, message.date},
		{u"edit_date"_q, message.editDate},
		{u"sender_id"_q, QString::number(message.senderId)},
		{u"topic_id"_q, QString::number(message.topicId)},
		{u"dialog_title"_q, message.dialogTitle},
		{u"dialog_type"_q, message.dialogType},
		{u"media_type"_q, message.mediaType},
		{u"cached_media"_q, !message.mediaPath.isEmpty()},
		{u"tags"_q, message.tags},
		{u"outgoing"_q, message.outgoing},
		{u"unread"_q, message.unread},
		{u"deleted"_q, message.deleted},
		{u"protected_reference"_q, message.protectedContent},
	};
	if (!message.protectedContent) {
		result.insert(u"text"_q, message.text);
		result.insert(u"media_name"_q, message.mediaName);
		result.insert(u"mime_type"_q, message.mimeType);
		result.insert(u"links"_q, message.links);
	}
	return result;
}

} // namespace

void Initialize() {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
}

void IndexMessage(not_null<HistoryItem *> item, bool unread) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	if (!Database || !Enabled(u"vault_enabled"_q, true) || !item->isRegular() || item->isLocal()) {
		return;
	}

	const auto protectedContent = IsProtected(item);
	const auto media = protectedContent ? MediaInfo() : ReadMedia(item);
	const auto text = protectedContent ? QString() : item->originalText().text;
	const auto links = protectedContent ? QString() : ExtractLinks(text);
	const auto edited = item->Get<HistoryMessageEdited>();
	const auto peer = item->history()->peer;
	const auto from = item->from();
	const auto topicId = item->topic() ? item->topicRootId().bare : 0;

	Statement statement(Database,
						"INSERT INTO "
						"vault_messages(account_id,dialog_id,message_id,date,edit_date,"
						"sender_id,topic_id,dialog_title,dialog_type,text,media_type,"
						"media_name,mime_type,media_path,"
						"links,outgoing,unread,deleted,protected,updated_at) "
						"VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?) "
						"ON CONFLICT(account_id,dialog_id,message_id) DO UPDATE SET "
						"date=excluded.date,edit_date=excluded.edit_date,sender_id="
						"excluded.sender_id,"
						"topic_id=excluded.topic_id,dialog_title=excluded.dialog_title,"
						"dialog_type=excluded.dialog_type,"
						"text=excluded.text,media_type=excluded.media_type,media_name="
						"excluded.media_name,"
						"mime_type=excluded.mime_type,media_path=excluded.media_path,"
						"links=excluded.links,outgoing="
						"excluded.outgoing,"
						"unread=max(vault_messages.unread,excluded.unread),deleted=0,"
						"protected=excluded.protected,updated_at=excluded.updated_at;");
	if (!statement) {
		return;
	}
	Bind(statement.get(), 1, AccountId(item));
	Bind(statement.get(), 2, getDialogIdFromPeer(peer));
	Bind(statement.get(), 3, item->id.bare);
	Bind(statement.get(), 4, int(item->date()));
	Bind(statement.get(), 5, edited ? int(edited->date) : 0);
	Bind(statement.get(), 6, ID(from->id.value & PeerId::kChatTypeMask));
	Bind(statement.get(), 7, ID(topicId));
	Bind(statement.get(), 8, peer->name());
	Bind(statement.get(), 9, DialogType(peer));
	Bind(statement.get(), 10, text);
	Bind(statement.get(), 11, media.type);
	Bind(statement.get(), 12, media.name);
	Bind(statement.get(), 13, media.mime);
	Bind(statement.get(), 14, media.path);
	Bind(statement.get(), 15, links);
	Bind(statement.get(), 16, item->out());
	Bind(statement.get(), 17, unread);
	Bind(statement.get(), 18, protectedContent);
	Bind(statement.get(), 19, int(base::unixtime::now()));
	if (sqlite3_step(statement.get()) != SQLITE_DONE) {
		LOG(("IntelGram Vault failed to index message: %1")
				.arg(QString::fromUtf8(sqlite3_errmsg(Database))));
		return;
	}

	auto record = MessageRecord();
	record.accountId = AccountId(item);
	record.dialogId = getDialogIdFromPeer(peer);
	record.messageId = item->id.bare;
	record.date = item->date();
	record.dialogType = DialogType(peer);
	record.text = text;
	record.mediaType = media.type;
	record.mediaName = media.name;
	record.mimeType = media.mime;
	record.mediaPath = media.path;
	record.links = links;
	record.outgoing = item->out();
	record.protectedContent = protectedContent;

	const auto unknown = !record.outgoing && Enabled(u"unknown_review"_q, true) && peer->asUser()
		&& !peer->asUser()->isBot() && !peer->asUser()->isContact();
	const auto suspiciousInvite = Enabled(u"invite_warnings"_q, true)
		&& (links.contains(u"t.me/+"_q, Qt::CaseInsensitive)
			|| links.contains(u"joinchat"_q, Qt::CaseInsensitive));
	if (unknown || suspiciousInvite) {
		auto automatic = QStringList();
		if (unknown) {
			automatic.push_back(u"unknown"_q);
		}
		if (suspiciousInvite) {
			automatic.push_back(u"suspicious"_q);
		}
		const auto tags = automatic.join(u' ');
		Statement automaticTags(Database,
								"UPDATE vault_messages SET tags=trim(tags || ' ' || ?) "
								"WHERE account_id=? AND dialog_id=? AND message_id=?;");
		if (automaticTags) {
			Bind(automaticTags.get(), 1, tags);
			Bind(automaticTags.get(), 2, record.accountId);
			Bind(automaticTags.get(), 3, record.dialogId);
			Bind(automaticTags.get(), 4, record.messageId);
			sqlite3_step(automaticTags.get());
		}
		record.tags = tags;
	}
	ApplyRules(record);
}

void RecordRevision(not_null<HistoryItem *> item) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	if (!Database || IsProtected(item) || item->isLocal()) {
		return;
	}
	const auto text = item->originalText().text;
	if (text.isEmpty()) {
		return;
	}
	Statement statement(Database,
						"INSERT OR IGNORE INTO vault_revisions(account_id,dialog_id,message_id,"
						"observed_at,text) VALUES(?,?,?,?,?);");
	if (!statement) {
		return;
	}
	Bind(statement.get(), 1, AccountId(item));
	Bind(statement.get(), 2, getDialogIdFromPeer(item->history()->peer));
	Bind(statement.get(), 3, item->id.bare);
	Bind(statement.get(), 4, int(base::unixtime::now()));
	Bind(statement.get(), 5, text);
	sqlite3_step(statement.get());
}

void MarkDeleted(not_null<HistoryItem *> item) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	if (!Database) {
		return;
	}
	Statement statement(Database,
						"UPDATE vault_messages SET deleted=1,updated_at=? "
						"WHERE account_id=? AND dialog_id=? AND message_id=?;");
	if (!statement) {
		return;
	}
	Bind(statement.get(), 1, int(base::unixtime::now()));
	Bind(statement.get(), 2, AccountId(item));
	Bind(statement.get(), 3, getDialogIdFromPeer(item->history()->peer));
	Bind(statement.get(), 4, item->id.bare);
	sqlite3_step(statement.get());
}

std::vector<SearchResult> Search(ID accountId, const QString &query, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<SearchResult>();
	if (!Database) {
		return result;
	}

	const auto fts = FtsQuery(query);
	const auto useFts = FtsAvailable && !fts.isEmpty();
	const auto sql = useFts ? QString(
								  "SELECT %1,snippet(vault_messages_fts,0,'<b>','</b>','...',18) "
								  "FROM vault_messages_fts JOIN vault_messages m ON "
								  "m.rowid=vault_messages_fts.rowid "
								  "WHERE vault_messages_fts MATCH ? AND (?=0 OR m.account_id=?) "
								  "ORDER BY bm25(vault_messages_fts),m.date DESC LIMIT ?;")
								  .arg(kSelectColumns)
		: query.trimmed().isEmpty()
		? QString(
			  "SELECT %1,'' FROM vault_messages m WHERE (?=0 OR "
			  "m.account_id=?) "
			  "ORDER BY m.date DESC LIMIT ?;")
			  .arg(kSelectColumns)
		: QString(
			  "SELECT %1,'' FROM vault_messages m WHERE (?=0 OR "
			  "m.account_id=?) AND "
			  "(m.text LIKE ? ESCAPE '\\' OR m.media_name LIKE ? ESCAPE '\\' "
			  "OR m.links LIKE ? ESCAPE '\\' OR m.dialog_title LIKE ? ESCAPE "
			  "'\\') "
			  "ORDER BY m.date DESC LIMIT ?;")
			  .arg(kSelectColumns);
	const auto utf8 = sql.toUtf8();
	Statement statement(Database, utf8.constData());
	if (!statement) {
		return result;
	}
	auto index = 1;
	if (useFts) {
		Bind(statement.get(), index++, fts);
	}
	Bind(statement.get(), index++, accountId);
	Bind(statement.get(), index++, accountId);
	if (!useFts && !query.trimmed().isEmpty()) {
		auto escaped = query;
		escaped.replace(u'\\', u"\\\\"_q);
		escaped.replace(u'%', u"\\%"_q);
		escaped.replace(u'_', u"\\_"_q);
		const auto pattern = u'%' + escaped + u'%';
		for (auto i = 0; i != 4; ++i) {
			Bind(statement.get(), index++, pattern);
		}
	}
	Bind(statement.get(), index, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		auto entry = SearchResult();
		static_cast<MessageRecord &>(entry) = ReadMessage(statement.get());
		entry.snippet = ColumnText(statement.get(), 19);
		if (entry.snippet.isEmpty()) {
			entry.snippet
				= entry.protectedContent ? u"Protected message reference"_q : entry.text.left(240);
		}
		result.push_back(std::move(entry));
	}
	return result;
}

std::vector<SearchResult> Messages(ID accountId, ID dialogId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<SearchResult>();
	if (!Database) {
		return result;
	}
	const auto sql = QString(
						 "SELECT %1,'' FROM vault_messages m WHERE (?=0 OR m.account_id=?) "
						 "AND (?=0 OR m.dialog_id=?) ORDER BY m.date DESC LIMIT ?;")
						 .arg(kSelectColumns);
	const auto utf8 = sql.toUtf8();
	Statement statement(Database, utf8.constData());
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, dialogId);
	Bind(statement.get(), 4, dialogId);
	Bind(statement.get(), 5, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		auto entry = SearchResult();
		static_cast<MessageRecord &>(entry) = ReadMessage(statement.get());
		entry.snippet
			= entry.protectedContent ? u"Protected message reference"_q : entry.text.left(240);
		result.push_back(std::move(entry));
	}
	return result;
}

std::vector<SearchResult> MessagesByIds(
		ID accountId,
		ID dialogId,
		const std::vector<int> &messageIds) {
	if (messageIds.empty()) {
		return {};
	}
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<SearchResult>();
	if (!Database) {
		return result;
	}
	auto placeholders = QStringList();
	placeholders.reserve(messageIds.size());
	for (auto i = 0; i != messageIds.size(); ++i) {
		placeholders.push_back(u"?"_q);
	}
	const auto sql = QString(
		"SELECT %1,'' FROM vault_messages m "
		"WHERE m.account_id=? AND m.dialog_id=? AND m.message_id IN (%2) "
		"ORDER BY m.date ASC;")
		.arg(kSelectColumns, placeholders.join(u','));
	const auto utf8 = sql.toUtf8();
	Statement statement(Database, utf8.constData());
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, dialogId);
	auto index = 3;
	for (const auto messageId : messageIds) {
		Bind(statement.get(), index++, messageId);
	}
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		auto entry = SearchResult();
		static_cast<MessageRecord &>(entry) = ReadMessage(statement.get());
		entry.snippet
			= entry.protectedContent ? u"Protected message reference"_q : entry.text.left(240);
		result.push_back(std::move(entry));
	}
	return result;
}

std::vector<SearchResult> SmartFolderMessages(ID accountId, SmartFolder folder, int limit) {
	if (folder == SmartFolder::All) {
		return Search(accountId, QString(), limit);
	}
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<SearchResult>();
	if (!Database) {
		return result;
	}

	auto condition = QString();
	switch (folder) {
		case SmartFolder::UnreadPeople:
			condition = u"m.unread=1 AND m.outgoing=0 AND m.dialog_type='person'"_q;
			break;
		case SmartFolder::Work:
			condition = u"(m.tags LIKE '%work%' OR p.tags LIKE '%work%')"_q;
			break;
		case SmartFolder::HighPriority:
			condition = u"p.priority>0"_q;
			break;
		case SmartFolder::NeedsReply:
			condition
				= u"m.outgoing=0 AND m.deleted=0 AND m.message_id=(SELECT vm.message_id "
				  "FROM vault_messages vm WHERE vm.account_id=m.account_id AND "
				  "vm.dialog_id=m.dialog_id "
				  "ORDER BY vm.date DESC LIMIT 1)"_q;
			break;
		case SmartFolder::SpamReview:
			condition
				= u"(m.tags LIKE '%spam%' OR m.tags LIKE '%unknown%' "
				  "OR m.tags LIKE '%suspicious%')"_q;
			break;
		case SmartFolder::All:
			break;
	}
	const auto sql = QString(
						 "SELECT %1,'' FROM vault_messages m LEFT JOIN vault_chat_policies p "
						 "ON p.account_id=m.account_id AND p.dialog_id=m.dialog_id "
						 "WHERE (?=0 OR m.account_id=?) AND %2 ORDER BY m.date DESC LIMIT ?;")
						 .arg(kSelectColumns, condition);
	const auto utf8 = sql.toUtf8();
	Statement statement(Database, utf8.constData());
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		auto entry = SearchResult();
		static_cast<MessageRecord &>(entry) = ReadMessage(statement.get());
		entry.snippet
			= entry.protectedContent ? u"Protected message reference"_q : entry.text.left(240);
		result.push_back(std::move(entry));
	}
	return result;
}

std::vector<SearchResult> MediaHistory(ID accountId, ID dialogId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<SearchResult>();
	if (!Database) {
		return result;
	}
	const auto sql = QString(
						 "SELECT %1,'' FROM vault_messages m WHERE (?=0 OR m.account_id=?) "
						 "AND (?=0 OR m.dialog_id=?) AND m.media_type<>'' "
						 "ORDER BY m.date DESC LIMIT ?;")
						 .arg(kSelectColumns);
	const auto utf8 = sql.toUtf8();
	Statement statement(Database, utf8.constData());
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, dialogId);
	Bind(statement.get(), 4, dialogId);
	Bind(statement.get(), 5, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		auto entry = SearchResult();
		static_cast<MessageRecord &>(entry) = ReadMessage(statement.get());
		entry.snippet = entry.protectedContent
			? u"Protected message reference"_q
			: (entry.mediaName.isEmpty() ? entry.mediaType : entry.mediaName);
		result.push_back(std::move(entry));
	}
	return result;
}

ConversationStats Stats(ID accountId, ID dialogId) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = ConversationStats();
	if (!Database) {
		return result;
	}
	Statement statement(Database,
						"SELECT count(*),sum(media_type<>''),sum(links<>''),sum(edit_date<>0),"
						"sum(deleted<>0),sum(protected<>0),min(date),max(date) FROM "
						"vault_messages "
						"WHERE (?=0 OR account_id=?) AND (?=0 OR dialog_id=?);");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, dialogId);
	Bind(statement.get(), 4, dialogId);
	if (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.messages = sqlite3_column_int(statement.get(), 0);
		result.media = sqlite3_column_int(statement.get(), 1);
		result.links = sqlite3_column_int(statement.get(), 2);
		result.edits = sqlite3_column_int(statement.get(), 3);
		result.deleted = sqlite3_column_int(statement.get(), 4);
		result.protectedReferences = sqlite3_column_int(statement.get(), 5);
		result.firstDate = sqlite3_column_int(statement.get(), 6);
		result.lastDate = sqlite3_column_int(statement.get(), 7);
	}
	return result;
}

int IndexedCount(ID accountId) {
	return Stats(accountId).messages;
}

ID AddMoment(not_null<HistoryItem *> item,
			 const QString &title,
			 const QString &note,
			 const QString &tags) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	if (!Database) {
		return 0;
	}
	Statement statement(Database,
						"INSERT INTO vault_moments(account_id,dialog_id,message_id,message_date,"
						"created_at,title,note,tags,protected_reference) "
						"VALUES(?,?,?,?,?,?,?,?,?);");
	if (!statement) {
		return 0;
	}
	Bind(statement.get(), 1, AccountId(item));
	Bind(statement.get(), 2, getDialogIdFromPeer(item->history()->peer));
	Bind(statement.get(), 3, item->id.bare);
	Bind(statement.get(), 4, int(item->date()));
	Bind(statement.get(), 5, int(base::unixtime::now()));
	Bind(statement.get(), 6, title);
	Bind(statement.get(), 7, note);
	Bind(statement.get(), 8, tags);
	Bind(statement.get(), 9, IsProtected(item));
	return (sqlite3_step(statement.get()) == SQLITE_DONE) ? sqlite3_last_insert_rowid(Database) : 0;
}

bool RemoveMoment(ID id) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	Statement statement(Database, "DELETE FROM vault_moments WHERE id=?;");
	if (!statement) {
		return false;
	}
	Bind(statement.get(), 1, id);
	return sqlite3_step(statement.get()) == SQLITE_DONE;
}

std::vector<MomentRecord> Moments(ID accountId, ID dialogId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<MomentRecord>();
	Statement statement(Database,
						"SELECT "
						"id,account_id,dialog_id,message_id,message_date,created_at,title,note,"
						"tags,"
						"protected_reference FROM vault_moments WHERE (?=0 OR account_id=?) "
						"AND (?=0 OR dialog_id=?) ORDER BY created_at DESC LIMIT ?;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, dialogId);
	Bind(statement.get(), 4, dialogId);
	Bind(statement.get(), 5, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(MomentRecord{
			.id = sqlite3_column_int64(statement.get(), 0),
			.accountId = sqlite3_column_int64(statement.get(), 1),
			.dialogId = sqlite3_column_int64(statement.get(), 2),
			.messageId = sqlite3_column_int(statement.get(), 3),
			.messageDate = sqlite3_column_int(statement.get(), 4),
			.createdAt = sqlite3_column_int(statement.get(), 5),
			.title = ColumnText(statement.get(), 6),
			.note = ColumnText(statement.get(), 7),
			.tags = ColumnText(statement.get(), 8),
			.protectedReference = sqlite3_column_int(statement.get(), 9) != 0,
		});
	}
	return result;
}

void SaveProfileNote(const ProfileNoteRecord &note) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	Statement statement(Database,
						"INSERT INTO "
						"vault_profile_notes(account_id,peer_id,note,tags,"
						"relationship,reminder_at,updated_at) "
						"VALUES(?,?,?,?,?,?,?) ON CONFLICT(account_id,peer_id) "
						"DO UPDATE SET note=excluded.note,"
						"tags=excluded.tags,relationship=excluded.relationship,"
						"reminder_at=excluded.reminder_at,"
						"updated_at=excluded.updated_at;");
	if (!statement) {
		return;
	}
	Bind(statement.get(), 1, note.accountId);
	Bind(statement.get(), 2, note.peerId);
	Bind(statement.get(), 3, note.note);
	Bind(statement.get(), 4, note.tags);
	Bind(statement.get(), 5, note.relationship);
	Bind(statement.get(), 6, note.reminderAt);
	const auto updatedAt = int(base::unixtime::now());
	Bind(statement.get(), 7, updatedAt);
	if (sqlite3_step(statement.get()) != SQLITE_DONE) {
		return;
	}
	Statement history(Database,
					  "INSERT INTO vault_profile_note_history("
					  "account_id,peer_id,note,tags,relationship,reminder_at,updated_at) "
					  "VALUES(?,?,?,?,?,?,?);");
	if (!history) {
		return;
	}
	Bind(history.get(), 1, note.accountId);
	Bind(history.get(), 2, note.peerId);
	Bind(history.get(), 3, note.note);
	Bind(history.get(), 4, note.tags);
	Bind(history.get(), 5, note.relationship);
	Bind(history.get(), 6, note.reminderAt);
	Bind(history.get(), 7, updatedAt);
	sqlite3_step(history.get());
}

std::optional<ProfileNoteRecord> ProfileNote(ID accountId, ID peerId) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	Statement statement(Database,
						"SELECT account_id,peer_id,note,tags,relationship,reminder_at,updated_at "
						"FROM vault_profile_notes WHERE account_id=? AND peer_id=?;");
	if (!statement) {
		return std::nullopt;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, peerId);
	if (sqlite3_step(statement.get()) != SQLITE_ROW) {
		return std::nullopt;
	}
	return ProfileNoteRecord{
		.accountId = sqlite3_column_int64(statement.get(), 0),
		.peerId = sqlite3_column_int64(statement.get(), 1),
		.note = ColumnText(statement.get(), 2),
		.tags = ColumnText(statement.get(), 3),
		.relationship = ColumnText(statement.get(), 4),
		.reminderAt = sqlite3_column_int(statement.get(), 5),
		.updatedAt = sqlite3_column_int(statement.get(), 6),
	};
}

std::vector<ProfileNoteRecord> ProfileNotes(ID accountId) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<ProfileNoteRecord>();
	Statement statement(Database,
						"SELECT account_id,peer_id,note,tags,relationship,reminder_at,updated_at "
						"FROM vault_profile_notes WHERE (?=0 OR account_id=?) ORDER BY "
						"updated_at DESC;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(ProfileNoteRecord{
			.accountId = sqlite3_column_int64(statement.get(), 0),
			.peerId = sqlite3_column_int64(statement.get(), 1),
			.note = ColumnText(statement.get(), 2),
			.tags = ColumnText(statement.get(), 3),
			.relationship = ColumnText(statement.get(), 4),
			.reminderAt = sqlite3_column_int(statement.get(), 5),
			.updatedAt = sqlite3_column_int(statement.get(), 6),
		});
	}
	return result;
}

std::vector<ProfileNoteRecord> ProfileNoteHistory(ID accountId, ID peerId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<ProfileNoteRecord>();
	Statement statement(Database,
						"SELECT account_id,peer_id,note,tags,relationship,reminder_at,updated_at "
						"FROM vault_profile_note_history WHERE (?=0 OR account_id=?) "
						"AND (?=0 OR peer_id=?) ORDER BY updated_at DESC LIMIT ?;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, peerId);
	Bind(statement.get(), 4, peerId);
	Bind(statement.get(), 5, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(ProfileNoteRecord{
			.accountId = sqlite3_column_int64(statement.get(), 0),
			.peerId = sqlite3_column_int64(statement.get(), 1),
			.note = ColumnText(statement.get(), 2),
			.tags = ColumnText(statement.get(), 3),
			.relationship = ColumnText(statement.get(), 4),
			.reminderAt = sqlite3_column_int(statement.get(), 5),
			.updatedAt = sqlite3_column_int(statement.get(), 6),
		});
	}
	return result;
}

void CaptureIdentity(not_null<PeerData *> peer) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	if (!Database) {
		return;
	}
	auto flags = QStringList();
	if (peer->isVerified()) {
		flags.push_back(u"verified"_q);
	}
	if (peer->isPremium()) {
		flags.push_back(u"premium"_q);
	}
	if (peer->isScam()) {
		flags.push_back(u"scam"_q);
	}
	Statement statement(Database,
						"INSERT INTO "
						"vault_identity_snapshots(account_id,peer_id,observed_at,display_name,"
						"usernames,public_flags,shared_groups) VALUES(?,?,?,?,?,?,?);");
	if (!statement) {
		return;
	}
	Bind(statement.get(), 1, AccountId(&peer->session()));
	Bind(statement.get(), 2, ID(peer->id.value & PeerId::kChatTypeMask));
	Bind(statement.get(), 3, int(base::unixtime::now()));
	Bind(statement.get(), 4, peer->name());
	auto usernames = QStringList();
	for (const auto &username : peer->usernames()) {
		usernames.push_back(username);
	}
	Bind(statement.get(), 5, usernames.join(u", "_q));
	Bind(statement.get(), 6, flags.join(u", "_q));
	Bind(statement.get(), 7, peer->asUser() ? peer->asUser()->commonChatsCount() : 0);
	sqlite3_step(statement.get());
}

std::vector<IdentitySnapshotRecord> IdentityHistory(ID accountId, ID peerId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<IdentitySnapshotRecord>();
	Statement statement(Database,
						"SELECT "
						"id,account_id,peer_id,observed_at,display_name,usernames,public_flags,"
						"shared_groups "
						"FROM vault_identity_snapshots WHERE account_id=? AND peer_id=? "
						"ORDER BY observed_at DESC LIMIT ?;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, peerId);
	Bind(statement.get(), 3, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(IdentitySnapshotRecord{
			.id = sqlite3_column_int64(statement.get(), 0),
			.accountId = sqlite3_column_int64(statement.get(), 1),
			.peerId = sqlite3_column_int64(statement.get(), 2),
			.observedAt = sqlite3_column_int(statement.get(), 3),
			.displayName = ColumnText(statement.get(), 4),
			.usernames = ColumnText(statement.get(), 5),
			.publicFlags = ColumnText(statement.get(), 6),
			.sharedGroups = sqlite3_column_int(statement.get(), 7),
		});
	}
	return result;
}

std::vector<IdentitySnapshotRecord> IdentitySnapshots(ID accountId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<IdentitySnapshotRecord>();
	Statement statement(Database,
						"SELECT "
						"id,account_id,peer_id,observed_at,display_name,usernames,public_flags,"
						"shared_groups FROM vault_identity_snapshots "
						"WHERE (?=0 OR account_id=?) ORDER BY observed_at DESC LIMIT ?;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(IdentitySnapshotRecord{
			.id = sqlite3_column_int64(statement.get(), 0),
			.accountId = sqlite3_column_int64(statement.get(), 1),
			.peerId = sqlite3_column_int64(statement.get(), 2),
			.observedAt = sqlite3_column_int(statement.get(), 3),
			.displayName = ColumnText(statement.get(), 4),
			.usernames = ColumnText(statement.get(), 5),
			.publicFlags = ColumnText(statement.get(), 6),
			.sharedGroups = sqlite3_column_int(statement.get(), 7),
		});
	}
	return result;
}

ID SaveRule(const RuleRecord &rule) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	if (!Database || rule.name.trimmed().isEmpty()
		|| (rule.match == RuleMatch::Keyword && rule.pattern.trimmed().isEmpty())) {
		return 0;
	}
	const auto update = rule.id != 0;
	Statement statement(
		Database,
		update ? "UPDATE vault_rules SET "
				 "account_id=?,name=?,match_type=?,pattern=?,action=?,action_value=?,enabled=? "
				 "WHERE id=?;"
			   : "INSERT INTO "
				 "vault_rules(account_id,name,match_type,pattern,action,action_value,enabled,"
				 "created_at) "
				 "VALUES(?,?,?,?,?,?,?,?);");
	if (!statement) {
		return 0;
	}
	Bind(statement.get(), 1, rule.accountId);
	Bind(statement.get(), 2, rule.name);
	Bind(statement.get(), 3, int(rule.match));
	Bind(statement.get(), 4, rule.pattern);
	Bind(statement.get(), 5, int(rule.action));
	Bind(statement.get(), 6, rule.actionValue);
	Bind(statement.get(), 7, rule.enabled);
	Bind(statement.get(), 8, update ? rule.id : ID(base::unixtime::now()));
	if (sqlite3_step(statement.get()) != SQLITE_DONE) {
		return 0;
	}
	return update ? rule.id : sqlite3_last_insert_rowid(Database);
}

bool DeleteRule(ID id) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	Statement statement(Database, "DELETE FROM vault_rules WHERE id=?;");
	if (!statement) {
		return false;
	}
	Bind(statement.get(), 1, id);
	return sqlite3_step(statement.get()) == SQLITE_DONE;
}

std::vector<RuleRecord> Rules(ID accountId) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<RuleRecord>();
	Statement statement(
		Database,
		"SELECT "
		"id,account_id,name,match_type,pattern,action,action_value,enabled,created_at "
		"FROM vault_rules WHERE (?=0 OR account_id=? OR account_id=0) "
		"ORDER BY created_at DESC;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(RuleRecord{
			.id = sqlite3_column_int64(statement.get(), 0),
			.accountId = sqlite3_column_int64(statement.get(), 1),
			.name = ColumnText(statement.get(), 2),
			.match = RuleMatch(sqlite3_column_int(statement.get(), 3)),
			.pattern = ColumnText(statement.get(), 4),
			.action = RuleAction(sqlite3_column_int(statement.get(), 5)),
			.actionValue = ColumnText(statement.get(), 6),
			.enabled = sqlite3_column_int(statement.get(), 7) != 0,
			.createdAt = sqlite3_column_int(statement.get(), 8),
		});
	}
	return result;
}

std::vector<RuleMatchRecord> RuleMatches(ID accountId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<RuleMatchRecord>();
	Statement statement(Database,
						"SELECT id,rule_id,account_id,dialog_id,message_id,matched_at,action,"
						"requires_confirmation,protected_reference FROM vault_rule_matches "
						"WHERE (?=0 OR account_id=?) ORDER BY matched_at DESC LIMIT ?;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(RuleMatchRecord{
			.id = sqlite3_column_int64(statement.get(), 0),
			.ruleId = sqlite3_column_int64(statement.get(), 1),
			.accountId = sqlite3_column_int64(statement.get(), 2),
			.dialogId = sqlite3_column_int64(statement.get(), 3),
			.messageId = sqlite3_column_int(statement.get(), 4),
			.matchedAt = sqlite3_column_int(statement.get(), 5),
			.action = ColumnText(statement.get(), 6),
			.requiresConfirmation = sqlite3_column_int(statement.get(), 7) != 0,
			.protectedReference = sqlite3_column_int(statement.get(), 8) != 0,
		});
	}
	return result;
}

std::vector<RevisionRecord> Revisions(ID accountId, ID dialogId, int messageId, int limit) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<RevisionRecord>();
	Statement statement(Database,
						"SELECT id,account_id,dialog_id,message_id,observed_at,text "
						"FROM vault_revisions WHERE (?=0 OR account_id=?) AND (?=0 OR dialog_id=?) "
						"AND (?=0 OR message_id=?) ORDER BY observed_at DESC LIMIT ?;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	Bind(statement.get(), 3, dialogId);
	Bind(statement.get(), 4, dialogId);
	Bind(statement.get(), 5, messageId);
	Bind(statement.get(), 6, messageId);
	Bind(statement.get(), 7, limit);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(RevisionRecord{
			.id = sqlite3_column_int64(statement.get(), 0),
			.accountId = sqlite3_column_int64(statement.get(), 1),
			.dialogId = sqlite3_column_int64(statement.get(), 2),
			.messageId = sqlite3_column_int(statement.get(), 3),
			.observedAt = sqlite3_column_int(statement.get(), 4),
			.text = ColumnText(statement.get(), 5),
		});
	}
	return result;
}

void SaveChatPolicy(const ChatPolicy &policy) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	Statement statement(Database,
						"INSERT INTO "
						"vault_chat_policies(account_id,dialog_id,tags,priority,download_mode,"
						"read_reminder,private_draft,local_mute) VALUES(?,?,?,?,?,?,?,?) "
						"ON CONFLICT(account_id,dialog_id) DO UPDATE SET tags=excluded.tags,"
						"priority=excluded.priority,download_mode=excluded.download_mode,"
						"read_reminder=excluded.read_reminder,private_draft=excluded.private_"
						"draft,"
						"local_mute=excluded.local_mute;");
	if (!statement) {
		return;
	}
	Bind(statement.get(), 1, policy.accountId);
	Bind(statement.get(), 2, policy.dialogId);
	Bind(statement.get(), 3, policy.tags);
	Bind(statement.get(), 4, policy.priority);
	Bind(statement.get(), 5, policy.downloadMode);
	Bind(statement.get(), 6, policy.readReminder);
	Bind(statement.get(), 7, policy.privateDraft);
	Bind(statement.get(), 8, policy.localMute);
	sqlite3_step(statement.get());
}

ChatPolicy GetChatPolicy(ID accountId, ID dialogId) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = ChatPolicy{.accountId = accountId, .dialogId = dialogId};
	Statement statement(Database,
						"SELECT "
						"tags,priority,download_mode,read_reminder,private_draft,local_mute "
						"FROM vault_chat_policies WHERE account_id=? AND dialog_id=?;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, dialogId);
	if (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.tags = ColumnText(statement.get(), 0);
		result.priority = sqlite3_column_int(statement.get(), 1);
		result.downloadMode = sqlite3_column_int(statement.get(), 2);
		result.readReminder = sqlite3_column_int(statement.get(), 3) != 0;
		result.privateDraft = sqlite3_column_int(statement.get(), 4) != 0;
		result.localMute = sqlite3_column_int(statement.get(), 5) != 0;
	}
	return result;
}

std::vector<ChatPolicy> ChatPolicies(ID accountId) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	auto result = std::vector<ChatPolicy>();
	Statement statement(Database,
						"SELECT account_id,dialog_id,tags,priority,download_mode,read_reminder,"
						"private_draft,local_mute FROM vault_chat_policies "
						"WHERE (?=0 OR account_id=?) ORDER BY account_id,dialog_id;");
	if (!statement) {
		return result;
	}
	Bind(statement.get(), 1, accountId);
	Bind(statement.get(), 2, accountId);
	while (sqlite3_step(statement.get()) == SQLITE_ROW) {
		result.push_back(ChatPolicy{
			.accountId = sqlite3_column_int64(statement.get(), 0),
			.dialogId = sqlite3_column_int64(statement.get(), 1),
			.tags = ColumnText(statement.get(), 2),
			.priority = sqlite3_column_int(statement.get(), 3),
			.downloadMode = sqlite3_column_int(statement.get(), 4),
			.readReminder = sqlite3_column_int(statement.get(), 5) != 0,
			.privateDraft = sqlite3_column_int(statement.get(), 6) != 0,
			.localMute = sqlite3_column_int(statement.get(), 7) != 0,
		});
	}
	return result;
}

bool SuppressNotification(not_null<HistoryItem *> item) {
	return GetChatPolicy(AccountId(item), getDialogIdFromPeer(item->history()->peer)).localMute;
}

bool SuppressNotifications(not_null<Data::Thread *> thread) {
	return GetChatPolicy(AccountId(&thread->owner().session()), getDialogIdFromPeer(thread->peer()))
		.localMute;
}

bool KeepDraftLocal(not_null<Data::Thread *> thread) {
	return Enabled(u"private_drafts"_q, true)
		&& GetChatPolicy(AccountId(&thread->owner().session()), getDialogIdFromPeer(thread->peer()))
			.privateDraft;
}

int DownloadMode(not_null<PeerData *> peer) {
	return GetChatPolicy(AccountId(&peer->session()), getDialogIdFromPeer(peer)).downloadMode;
}

void SetOption(const QString &key, const QString &value) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	Statement statement(Database,
						"INSERT INTO vault_options(key,value) VALUES(?,?) "
						"ON CONFLICT(key) DO UPDATE SET value=excluded.value;");
	if (!statement) {
		return;
	}
	Bind(statement.get(), 1, key);
	Bind(statement.get(), 2, value);
	sqlite3_step(statement.get());
}

QString Option(const QString &key, const QString &fallback) {
	const auto lock = std::lock_guard(DatabaseMutex);
	EnsureInitialized();
	Statement statement(Database, "SELECT value FROM vault_options WHERE key=?;");
	if (!statement) {
		return fallback;
	}
	Bind(statement.get(), 1, key);
	return (sqlite3_step(statement.get()) == SQLITE_ROW) ? ColumnText(statement.get(), 0)
														 : fallback;
}

void SetEnabled(const QString &key, bool enabled) {
	SetOption(key, enabled ? u"1"_q : u"0"_q);
}

bool Enabled(const QString &key, bool fallback) {
	return Option(key, fallback ? u"1"_q : u"0"_q) == u"1"_q;
}

QByteArray ExportJson(
		ID accountId,
		ID dialogId,
		const std::vector<int> &messageIds) {
	const auto selected = !messageIds.empty();
	const auto records = selected
		? MessagesByIds(accountId, dialogId, messageIds)
		: Messages(accountId, dialogId, 1000000);
	auto messages = QJsonArray();
	for (const auto &message : records) {
		messages.push_back(MessageJson(message));
	}
	const auto included = [&](int messageId) {
		return !selected
			|| std::find(messageIds.begin(), messageIds.end(), messageId) != messageIds.end();
	};
	auto moments = QJsonArray();
	for (const auto &moment : Moments(accountId, dialogId, 1000000)) {
		if (!included(moment.messageId)) {
			continue;
		}
		moments.push_back(QJsonObject{
			{u"id"_q, QString::number(moment.id)},
			{u"dialog_id"_q, QString::number(moment.dialogId)},
			{u"message_id"_q, moment.messageId},
			{u"message_date"_q, moment.messageDate},
			{u"created_at"_q, moment.createdAt},
			{u"title"_q, moment.title},
			{u"note"_q, moment.note},
			{u"tags"_q, moment.tags},
			{u"protected_reference"_q, moment.protectedReference},
		});
	}
	auto notes = QJsonArray();
	auto noteHistory = QJsonArray();
	if (!dialogId) {
		for (const auto &note : ProfileNotes(accountId)) {
			notes.push_back(QJsonObject{
				{u"peer_id"_q, QString::number(note.peerId)},
				{u"note"_q, note.note},
				{u"tags"_q, note.tags},
				{u"relationship"_q, note.relationship},
				{u"reminder_at"_q, note.reminderAt},
				{u"updated_at"_q, note.updatedAt},
			});
		}
		for (const auto &note : ProfileNoteHistory(accountId, 0, 1000000)) {
			noteHistory.push_back(QJsonObject{
				{u"peer_id"_q, QString::number(note.peerId)},
				{u"note"_q, note.note},
				{u"tags"_q, note.tags},
				{u"relationship"_q, note.relationship},
				{u"reminder_at"_q, note.reminderAt},
				{u"updated_at"_q, note.updatedAt},
			});
		}
	}
	auto revisions = QJsonArray();
	for (const auto &revision : Revisions(accountId, dialogId, 0, 1000000)) {
		if (!included(revision.messageId)) {
			continue;
		}
		revisions.push_back(QJsonObject{
			{u"id"_q, QString::number(revision.id)},
			{u"dialog_id"_q, QString::number(revision.dialogId)},
			{u"message_id"_q, revision.messageId},
			{u"observed_at"_q, revision.observedAt},
			{u"text"_q, revision.text},
		});
	}
	auto identities = QJsonArray();
	auto rules = QJsonArray();
	auto ruleMatches = QJsonArray();
	if (!dialogId) {
		for (const auto &identity : IdentitySnapshots(accountId, 1000000)) {
			identities.push_back(QJsonObject{
				{u"peer_id"_q, QString::number(identity.peerId)},
				{u"observed_at"_q, identity.observedAt},
				{u"display_name"_q, identity.displayName},
				{u"usernames"_q, identity.usernames},
				{u"public_flags"_q, identity.publicFlags},
				{u"shared_groups"_q, identity.sharedGroups},
			});
		}
		for (const auto &rule : Rules(accountId)) {
			rules.push_back(QJsonObject{
				{u"id"_q, QString::number(rule.id)},
				{u"account_id"_q, QString::number(rule.accountId)},
				{u"name"_q, rule.name},
				{u"match_type"_q, int(rule.match)},
				{u"pattern"_q, rule.pattern},
				{u"action"_q, int(rule.action)},
				{u"action_value"_q, rule.actionValue},
				{u"enabled"_q, rule.enabled},
				{u"created_at"_q, rule.createdAt},
			});
		}
		for (const auto &match : RuleMatches(accountId, 1000000)) {
			ruleMatches.push_back(QJsonObject{
				{u"rule_id"_q, QString::number(match.ruleId)},
				{u"dialog_id"_q, QString::number(match.dialogId)},
				{u"message_id"_q, match.messageId},
				{u"matched_at"_q, match.matchedAt},
				{u"action"_q, match.action},
				{u"requires_confirmation"_q, match.requiresConfirmation},
				{u"protected_reference"_q, match.protectedReference},
			});
		}
	}
	auto policies = QJsonArray();
	for (const auto &policy : ChatPolicies(accountId)) {
		if (dialogId && policy.dialogId != dialogId) {
			continue;
		}
		policies.push_back(QJsonObject{
			{u"dialog_id"_q, QString::number(policy.dialogId)},
			{u"tags"_q, policy.tags},
			{u"priority"_q, policy.priority},
			{u"download_mode"_q, policy.downloadMode},
			{u"read_reminder"_q, policy.readReminder},
			{u"private_draft"_q, policy.privateDraft},
			{u"local_mute"_q, policy.localMute},
		});
	}
	auto options = QJsonObject();
	if (!dialogId) {
		const auto lock = std::lock_guard(DatabaseMutex);
		EnsureInitialized();
		Statement statement(Database, "SELECT key,value FROM vault_options ORDER BY key;");
		if (statement) {
			while (sqlite3_step(statement.get()) == SQLITE_ROW) {
				options.insert(ColumnText(statement.get(), 0), ColumnText(statement.get(), 1));
			}
		}
	}
	return QJsonDocument(
			   QJsonObject{
				   {u"format"_q,
					selected
						? u"IntelGram Message Selection"_q
						: dialogId
						? u"IntelGram Chat Export"_q
						: u"IntelGram Account Archive"_q},
				   {u"version"_q, 2},
				   {u"created_at"_q, int(base::unixtime::now())},
				   {u"account_id"_q, QString::number(accountId)},
				   {u"scope_dialog_id"_q, QString::number(dialogId)},
				   {u"protected_content_policy"_q,
					u"Protected chats contain references and metadata only."_q},
				   {u"messages"_q, messages},
				   {u"moments"_q, moments},
				   {u"profile_notes"_q, notes},
				   {u"profile_note_history"_q, noteHistory},
				   {u"public_identity_snapshots"_q, identities},
				   {u"rules"_q, rules},
				   {u"rule_activity"_q, ruleMatches},
				   {u"chat_tools_and_folders"_q, policies},
				   {u"vault_options"_q, options},
				   {u"message_revisions"_q, revisions},
			   })
		.toJson(QJsonDocument::Indented);
}

QByteArray ExportMarkdown(
		ID accountId,
		ID dialogId,
		const std::vector<int> &messageIds) {
	const auto selected = !messageIds.empty();
	const auto records = selected
		? MessagesByIds(accountId, dialogId, messageIds)
		: Messages(accountId, dialogId, 1000000);
	auto result = selected
		? u"# IntelGram Message Selection\n\n"_q
		: dialogId
		? u"# IntelGram Chat Export\n\n"_q
		: u"# IntelGram Account Archive\n\n"_q;
	result += u"Protected chats are represented by local jump references only.\n\n"_q;
	for (const auto &message : records) {
		result += u"## "_q + message.dialogTitle + u" · "_q + QString::number(message.messageId)
			+ u"\n\n"_q;
		result += u"- Date: "_q + QString::number(message.date) + u"\n"_q;
		result += u"- Account: "_q + QString::number(message.accountId) + u"\n"_q;
		result += u"- Dialog: "_q + QString::number(message.dialogId) + u"\n"_q;
		if (message.protectedContent) {
			result += u"- Protected reference: yes\n\n"_q;
		} else {
			if (!message.mediaName.isEmpty() || !message.mediaType.isEmpty()) {
				result += u"- Media: "_q
					+ (message.mediaName.isEmpty() ? message.mediaType : message.mediaName)
					+ u"\n"_q;
			}
			if (!message.links.isEmpty()) {
				result += u"- Links: "_q + message.links + u"\n"_q;
			}
			if (message.deleted) {
				result += u"- Locally observed as deleted: yes\n"_q;
			}
			result += u"\n"_q + message.text + u"\n\n"_q;
		}
	}
	result += u"# Message revisions\n\n"_q;
	for (const auto &revision : Revisions(accountId, dialogId, 0, 1000000)) {
		if (selected
			&& std::find(messageIds.begin(), messageIds.end(), revision.messageId)
				== messageIds.end()) {
			continue;
		}
		result += u"## Dialog "_q + QString::number(revision.dialogId) + u" · message "_q
			+ QString::number(revision.messageId) + u"\n\n"_q;
		result += revision.text + u"\n\n"_q;
	}
	return result.toUtf8();
}

QByteArray ExportHtml(
		ID accountId,
		ID dialogId,
		const std::vector<int> &messageIds) {
	const auto selected = !messageIds.empty();
	const auto records = selected
		? MessagesByIds(accountId, dialogId, messageIds)
		: Messages(accountId, dialogId, 1000000);
	auto result = QString(
		u"<!doctype html><html><head><meta charset=\"utf-8\"><title>IntelGram "
		u"Backup</title>"
		u"<style>body{font:16px system-ui;max-width:960px;margin:40px "
		u"auto;padding:0 20px;"
		u"background:#111820;color:#e8edf2}article{border-bottom:1px solid "
		u"#34404c;padding:18px 0}"
		u"small{color:#8ea0b3}.protected{color:#f2bd5a}</style></head><body>"_q);
	result += selected
		? u"<h1>IntelGram Message Selection</h1>"_q
		: dialogId
		? u"<h1>IntelGram Chat Export</h1>"_q
		: u"<h1>IntelGram Account Archive</h1>"_q;
	result += u"<p>Protected chats contain references and metadata only.</p>"_q;
	for (const auto &message : records) {
		result += u"<article><h2>"_q + message.dialogTitle.toHtmlEscaped() + u" <small>#"_q
			+ QString::number(message.messageId) + u"</small></h2>"_q;
		result += u"<small>Account "_q + QString::number(message.accountId) + u" · Dialog "_q
			+ QString::number(message.dialogId) + u" · Date "_q + QString::number(message.date)
			+ u"</small>"_q;
		result += message.protectedContent
			? u"<p class=\"protected\">Protected message reference</p>"_q
			: u"<p>"_q + message.text.toHtmlEscaped().replace(u'\n', u"<br>"_q) + u"</p>"_q;
		if (!message.protectedContent && (!message.mediaName.isEmpty() || !message.links.isEmpty())) {
			result += u"<small>"_q;
			if (!message.mediaName.isEmpty()) {
				result += u"Media: "_q + message.mediaName.toHtmlEscaped() + u" "_q;
			}
			if (!message.links.isEmpty()) {
				result += u"Links: "_q + message.links.toHtmlEscaped();
			}
			result += u"</small>"_q;
		}
		result += u"</article>"_q;
	}
	result += u"<h1>Message revisions</h1>"_q;
	for (const auto &revision : Revisions(accountId, dialogId, 0, 1000000)) {
		if (selected
			&& std::find(messageIds.begin(), messageIds.end(), revision.messageId)
				== messageIds.end()) {
			continue;
		}
		result += u"<article><h2>Dialog "_q + QString::number(revision.dialogId) + u" · message "_q
			+ QString::number(revision.messageId) + u"</h2><p>"_q
			+ revision.text.toHtmlEscaped().replace(u'\n', u"<br>"_q) + u"</p></article>"_q;
	}
	result += u"</body></html>"_q;
	return result.toUtf8();
}

} // namespace IntelGram::Vault
