// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <optional>
#include <vector>

class HistoryItem;
class PeerData;

namespace Data {
class Thread;
} // namespace Data

namespace IntelGram::Vault {

using ID = long long;

enum class SmartFolder {
	All,
	UnreadPeople,
	Work,
	HighPriority,
	NeedsReply,
	SpamReview,
};

enum class RuleMatch {
	Keyword,
	Link,
	Photo,
	File,
};

enum class RuleAction {
	Tag,
	Save,
	Notify,
	MuteLocally,
	QueueForward,
	MarkSpam,
};

struct MessageRecord {
	ID accountId = 0;
	ID dialogId = 0;
	int messageId = 0;
	int date = 0;
	int editDate = 0;
	ID senderId = 0;
	ID topicId = 0;
	QString dialogTitle;
	QString dialogType;
	QString text;
	QString mediaType;
	QString mediaName;
	QString mimeType;
	QString mediaPath;
	QString links;
	QString tags;
	bool outgoing = false;
	bool unread = false;
	bool deleted = false;
	bool protectedContent = false;
};

struct SearchResult : MessageRecord {
	QString snippet;
};

struct ConversationStats {
	int messages = 0;
	int media = 0;
	int links = 0;
	int edits = 0;
	int deleted = 0;
	int protectedReferences = 0;
	int firstDate = 0;
	int lastDate = 0;
};

struct MomentRecord {
	ID id = 0;
	ID accountId = 0;
	ID dialogId = 0;
	int messageId = 0;
	int messageDate = 0;
	int createdAt = 0;
	QString title;
	QString note;
	QString tags;
	bool protectedReference = false;
};

struct ProfileNoteRecord {
	ID accountId = 0;
	ID peerId = 0;
	QString note;
	QString tags;
	QString relationship;
	int reminderAt = 0;
	int updatedAt = 0;
};

struct IdentitySnapshotRecord {
	ID id = 0;
	ID accountId = 0;
	ID peerId = 0;
	int observedAt = 0;
	QString displayName;
	QString usernames;
	QString publicFlags;
	int sharedGroups = 0;
};

struct RuleRecord {
	ID id = 0;
	ID accountId = 0;
	QString name;
	RuleMatch match = RuleMatch::Keyword;
	QString pattern;
	RuleAction action = RuleAction::Tag;
	QString actionValue;
	bool enabled = true;
	int createdAt = 0;
};

struct RevisionRecord {
	ID id = 0;
	ID accountId = 0;
	ID dialogId = 0;
	int messageId = 0;
	int observedAt = 0;
	QString text;
};

struct RuleMatchRecord {
	ID id = 0;
	ID ruleId = 0;
	ID accountId = 0;
	ID dialogId = 0;
	int messageId = 0;
	int matchedAt = 0;
	QString action;
	bool requiresConfirmation = false;
	bool protectedReference = false;
};

struct ChatPolicy {
	ID accountId = 0;
	ID dialogId = 0;
	QString tags;
	int priority = 0;
	int downloadMode = 0;
	bool readReminder = false;
	bool privateDraft = false;
	bool localMute = false;
};

void Initialize();
void IndexMessage(not_null<HistoryItem *> item, bool unread = false);
void RecordRevision(not_null<HistoryItem *> item);
void MarkDeleted(not_null<HistoryItem *> item);

[[nodiscard]] std::vector<SearchResult> Search(ID accountId, const QString &query, int limit = 100);
[[nodiscard]] std::vector<SearchResult> Messages(ID accountId, ID dialogId = 0, int limit = 100);
[[nodiscard]] std::vector<SearchResult> MessagesByIds(ID accountId,
													  ID dialogId,
													  const std::vector<int> &messageIds);
[[nodiscard]] std::vector<SearchResult> SmartFolderMessages(ID accountId,
															SmartFolder folder,
															int limit = 100);
[[nodiscard]] std::vector<SearchResult> MediaHistory(ID accountId,
													 ID dialogId = 0,
													 int limit = 200);
[[nodiscard]] ConversationStats Stats(ID accountId, ID dialogId = 0);
[[nodiscard]] int IndexedCount(ID accountId = 0);

[[nodiscard]] ID AddMoment(not_null<HistoryItem *> item,
						   const QString &title,
						   const QString &note,
						   const QString &tags);
[[nodiscard]] bool RemoveMoment(ID id);
[[nodiscard]] std::vector<MomentRecord> Moments(ID accountId, ID dialogId = 0, int limit = 200);

void SaveProfileNote(const ProfileNoteRecord &note);
[[nodiscard]] std::optional<ProfileNoteRecord> ProfileNote(ID accountId, ID peerId);
[[nodiscard]] std::vector<ProfileNoteRecord> ProfileNotes(ID accountId);
[[nodiscard]] std::vector<ProfileNoteRecord> ProfileNoteHistory(ID accountId,
																ID peerId = 0,
																int limit = 1000);

void CaptureIdentity(not_null<PeerData *> peer);
[[nodiscard]] std::vector<IdentitySnapshotRecord> IdentityHistory(ID accountId,
																  ID peerId,
																  int limit = 100);
[[nodiscard]] std::vector<IdentitySnapshotRecord> IdentitySnapshots(ID accountId,
																	int limit = 10000);

[[nodiscard]] ID SaveRule(const RuleRecord &rule);
[[nodiscard]] bool DeleteRule(ID id);
[[nodiscard]] std::vector<RuleRecord> Rules(ID accountId = 0);
[[nodiscard]] std::vector<RuleMatchRecord> RuleMatches(ID accountId, int limit = 200);
[[nodiscard]] std::vector<RevisionRecord> Revisions(ID accountId,
													ID dialogId = 0,
													int messageId = 0,
													int limit = 500);

void SaveChatPolicy(const ChatPolicy &policy);
[[nodiscard]] ChatPolicy GetChatPolicy(ID accountId, ID dialogId);
[[nodiscard]] std::vector<ChatPolicy> ChatPolicies(ID accountId);
[[nodiscard]] bool SuppressNotification(not_null<HistoryItem *> item);
[[nodiscard]] bool SuppressNotifications(not_null<Data::Thread *> thread);
[[nodiscard]] bool KeepDraftLocal(not_null<Data::Thread *> thread);
[[nodiscard]] int DownloadMode(not_null<PeerData *> peer);

void SetOption(const QString &key, const QString &value);
[[nodiscard]] QString Option(const QString &key, const QString &fallback = QString());
void SetEnabled(const QString &key, bool enabled);
[[nodiscard]] bool Enabled(const QString &key, bool fallback = false);

[[nodiscard]] QByteArray ExportJson(ID accountId,
									ID dialogId = 0,
									const std::vector<int> &messageIds = {});
[[nodiscard]] QByteArray ExportMarkdown(ID accountId,
										ID dialogId = 0,
										const std::vector<int> &messageIds = {});
[[nodiscard]] QByteArray ExportHtml(ID accountId,
									ID dialogId = 0,
									const std::vector<int> &messageIds = {});

} // namespace IntelGram::Vault
