// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/data/intelgram_export.h"

#include "ayu/utils/telegram_helpers.h"
#include "base/unixtime.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "main/main_session.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>
#include <QtCore/QTemporaryDir>
#include <QtGui/QPageSize>
#include <QtGui/QPdfWriter>
#include <QtGui/QTextDocument>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <zip.h>
#include <zlib.h>

namespace IntelGram::Export {
namespace {

constexpr auto kArchiveIterations = 250000;
constexpr auto kSaltSize = 16;
constexpr auto kNonceSize = 12;
constexpr auto kTagSize = 16;
constexpr auto kMediaChunkSize = 256 * 1024;

[[nodiscard]] bool WriteFile(const QString &path, const QByteArray &data, QString &error) {
	auto file = QSaveFile(path);
	if (!file.open(QIODevice::WriteOnly)) {
		error = file.errorString();
		return false;
	}
	if (file.write(data) != data.size() || !file.commit()) {
		error = file.errorString();
		return false;
	}
	return true;
}

[[nodiscard]] bool WritePdf(const QString &path, const QByteArray &html, QString &error) {
	auto writer = QPdfWriter(path);
	writer.setPageSize(QPageSize(QPageSize::A4));
	writer.setTitle(u"IntelGram Local Export"_q);
	writer.setCreator(u"IntelGram"_q);
	auto document = QTextDocument();
	document.setHtml(QString::fromUtf8(html));
	document.print(&writer);
	if (!QFile::exists(path) || QFileInfo(path).size() == 0) {
		error = u"Could not create the PDF export."_q;
		return false;
	}
	return true;
}

[[nodiscard]] QByteArray AddContacts(QByteArray json, Vault::ID accountId) {
	const auto session = getSession(accountId);
	if (!session) {
		return json;
	}
	auto document = QJsonDocument::fromJson(json);
	if (!document.isObject()) {
		return json;
	}
	auto contacts = QJsonArray();
	session->data().enumerateUsers([&](not_null<UserData *> user) {
		if (!user->isSelf() && !user->isContact()) {
			return;
		}
		auto usernames = QJsonArray();
		for (const auto &username : user->usernames()) {
			usernames.push_back(username);
		}
		contacts.push_back(QJsonObject{
			{u"id"_q, QString::number(user->id.value & PeerId::kChatTypeMask)},
			{u"name"_q, user->name()},
			{u"usernames"_q, usernames},
			{u"phone"_q, user->phone()},
			{u"self"_q, user->isSelf()},
			{u"bot"_q, user->isBot()},
		});
	});
	auto root = document.object();
	root.insert(u"contacts"_q, contacts);
	return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

[[nodiscard]] bool AddZipEntry(zipFile archive,
							   const QString &name,
							   const QByteArray &data,
							   QString &error) {
	auto info = zip_fileinfo();
	const auto utf8Name = name.toUtf8();
	const auto opened
		= zipOpenNewFileInZip64(archive, utf8Name.constData(), &info, nullptr, 0, nullptr, 0,
								nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 1);
	if (opened != ZIP_OK) {
		error = u"Could not add a file to the ZIP archive."_q;
		return false;
	}
	const auto written = zipWriteInFileInZip(archive, data.constData(), unsigned(data.size()));
	const auto closed = zipCloseFileInZip(archive);
	if (written != ZIP_OK || closed != ZIP_OK) {
		error = u"Could not finish a file in the ZIP archive."_q;
		return false;
	}
	return true;
}

[[nodiscard]] QString MediaArchiveName(const Vault::MessageRecord &message) {
	auto name = QFileInfo(message.mediaPath).fileName();
	if (name.isEmpty()) {
		name = message.mediaName;
	}
	if (name.isEmpty()) {
		name = u"media.bin"_q;
	}
	for (auto i = 0; i != name.size(); ++i) {
		const auto character = name.at(i);
		if (!character.isLetterOrNumber() && character != u'.' && character != u'-'
			&& character != u'_') {
			name[i] = u'_';
		}
	}
	return u"media/account-"_q + QString::number(message.accountId) + u"/dialog-"_q
		+ QString::number(message.dialogId) + u'/' + QString::number(message.messageId) + u'-'
		+ name;
}

[[nodiscard]] bool AddZipFile(zipFile archive,
							  const QString &name,
							  const QString &path,
							  QString &error) {
	auto file = QFile(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return true;
	}
	auto info = zip_fileinfo();
	const auto utf8Name = name.toUtf8();
	if (zipOpenNewFileInZip64(archive, utf8Name.constData(), &info, nullptr, 0, nullptr, 0, nullptr,
							  Z_DEFLATED, Z_DEFAULT_COMPRESSION, 1)
		!= ZIP_OK) {
		error = u"Could not add downloaded media to the ZIP archive."_q;
		return false;
	}
	auto success = true;
	while (!file.atEnd()) {
		const auto chunk = file.read(kMediaChunkSize);
		if (chunk.isEmpty() && file.error() != QFile::NoError) {
			success = false;
			break;
		}
		if (!chunk.isEmpty()
			&& zipWriteInFileInZip(archive, chunk.constData(), unsigned(chunk.size())) != ZIP_OK) {
			success = false;
			break;
		}
	}
	if (zipCloseFileInZip(archive) != ZIP_OK) {
		success = false;
	}
	if (!success) {
		error = u"Could not copy downloaded media into the ZIP archive."_q;
	}
	return success;
}

[[nodiscard]] bool CreateZip(const QString &path,
							 Vault::ID accountId,
							 Vault::ID dialogId,
							 const std::vector<int> &messageIds,
							 const QByteArray &json,
							 const QByteArray &markdown,
							 const QByteArray &html,
							 const QByteArray &pdf,
							 QString &error) {
	const auto encoded = QFile::encodeName(path);
	const auto archive = zipOpen64(encoded.constData(), APPEND_STATUS_CREATE);
	if (!archive) {
		error = u"Could not create the ZIP archive."_q;
		return false;
	}
	const auto close = gsl::finally([&] { zipClose(archive, nullptr); });
	if (!AddZipEntry(archive, u"intelgram-backup.json"_q, json, error)
		|| !AddZipEntry(archive, u"intelgram-backup.md"_q, markdown, error)
		|| !AddZipEntry(archive, u"intelgram-backup.html"_q, html, error)) {
		return false;
	}
	if (!pdf.isEmpty() && !AddZipEntry(archive, u"intelgram-backup.pdf"_q, pdf, error)) {
		return false;
	}
	const auto settingsPath = u"./tdata/ayu_settings.json"_q;
	auto settings = QFile(settingsPath);
	if (settings.open(QIODevice::ReadOnly)
		&& !AddZipEntry(archive, u"settings/ayu_settings.json"_q, settings.readAll(), error)) {
		return false;
	}
	const auto messages = messageIds.empty()
		? Vault::Messages(accountId, dialogId, 1000000)
		: Vault::MessagesByIds(accountId, dialogId, messageIds);
	for (const auto &message : messages) {
		if (message.protectedContent || message.mediaPath.isEmpty()
			|| !QFileInfo(message.mediaPath).isFile()) {
			continue;
		}
		if (!AddZipFile(archive, MediaArchiveName(message), message.mediaPath, error)) {
			return false;
		}
	}
	return true;
}

[[nodiscard]] bool EncryptFile(const QString &source,
							   const QString &target,
							   const QString &passphrase,
							   QString &error) {
	auto salt = QByteArray(kSaltSize, Qt::Uninitialized);
	auto nonce = QByteArray(kNonceSize, Qt::Uninitialized);
	auto tag = QByteArray(kTagSize, Qt::Uninitialized);
	auto key = QByteArray(32, Qt::Uninitialized);
	const auto clearKey = gsl::finally([&] { key.fill('\0'); });
	if (RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), salt.size()) != 1
		|| RAND_bytes(reinterpret_cast<unsigned char *>(nonce.data()), nonce.size()) != 1) {
		error = u"Could not generate encryption randomness."_q;
		return false;
	}
	const auto password = passphrase.toUtf8();
	if (PKCS5_PBKDF2_HMAC(password.constData(), password.size(),
						  reinterpret_cast<const unsigned char *>(salt.constData()), salt.size(),
						  kArchiveIterations, EVP_sha256(), key.size(),
						  reinterpret_cast<unsigned char *>(key.data()))
		!= 1) {
		error = u"Could not derive the archive key."_q;
		return false;
	}
	const auto context = EVP_CIPHER_CTX_new();
	if (!context) {
		error = u"Could not initialize archive encryption."_q;
		return false;
	}
	const auto freeContext = gsl::finally([&] { EVP_CIPHER_CTX_free(context); });
	auto input = QFile(source);
	auto output = QSaveFile(target);
	if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) {
		error = input.isOpen() ? output.errorString() : input.errorString();
		return false;
	}
	const auto header = QByteArray("INTELGRAM-VAULT\x01", 16);
	const auto prefix = header + salt + nonce + tag;
	if (output.write(prefix) != prefix.size()) {
		error = output.errorString();
		return false;
	}
	if (EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1
		|| EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) != 1
		|| EVP_EncryptInit_ex(context, nullptr, nullptr,
								  reinterpret_cast<const unsigned char *>(key.constData()),
								  reinterpret_cast<const unsigned char *>(nonce.constData()))
			!= 1) {
		error = u"Could not encrypt the archive."_q;
		return false;
	}
	while (!input.atEnd()) {
		const auto plain = input.read(kMediaChunkSize);
		if (plain.isEmpty() && input.error() != QFile::NoError) {
			error = input.errorString();
			return false;
		}
		auto cipher = QByteArray(plain.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
		auto written = 0;
		if (!plain.isEmpty()
			&& EVP_EncryptUpdate(
				   context,
				   reinterpret_cast<unsigned char *>(cipher.data()),
				   &written,
				   reinterpret_cast<const unsigned char *>(plain.constData()),
				   plain.size())
				!= 1) {
			error = u"Could not encrypt the archive."_q;
			return false;
		}
		if (written && output.write(cipher.constData(), written) != written) {
			error = output.errorString();
			return false;
		}
	}
	auto final = QByteArray(EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
	auto finalWritten = 0;
	if (EVP_EncryptFinal_ex(context, reinterpret_cast<unsigned char *>(final.data()), &finalWritten)
			!= 1
		|| EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data()) != 1
		|| (finalWritten && output.write(final.constData(), finalWritten) != finalWritten)) {
		error = u"Could not finish archive encryption."_q;
		return false;
	}
	if (!output.seek(header.size() + salt.size() + nonce.size())
		|| output.write(tag) != tag.size() || !output.commit()) {
		error = output.errorString();
		return false;
	}
	return true;
}

} // namespace

Result Create(Vault::ID accountId, const QString &directory, const Options &options) {
	auto result = Result();
	if (directory.isEmpty()) {
		result.error = u"Choose an export folder."_q;
		return result;
	}
	if (options.encrypted && options.passphrase.size() < 8) {
		result.error = u"Encrypted backups require a passphrase of at least 8 characters."_q;
		return result;
	}
	auto target = QDir(directory);
	if (!target.exists() && !QDir().mkpath(directory)) {
		result.error = u"Could not create the export folder."_q;
		return result;
	}
	const auto stamp = QString::number(base::unixtime::now());
	const auto baseName = options.encrypted
		? u"IntelGram-Frozen-Backup-"_q + stamp
		: !options.messageIds.empty()
		? u"IntelGram-Message-Selection-"_q + stamp
		: options.dialogId
		? u"IntelGram-Chat-Export-"_q + stamp
		: u"IntelGram-Account-Export-"_q + stamp;
	const auto json = (options.dialogId || !options.messageIds.empty())
		? Vault::ExportJson(accountId, options.dialogId, options.messageIds)
		: AddContacts(Vault::ExportJson(accountId), accountId);
	const auto markdown
		= Vault::ExportMarkdown(accountId, options.dialogId, options.messageIds);
	const auto html = Vault::ExportHtml(accountId, options.dialogId, options.messageIds);

	auto temporary = QTemporaryDir();
	if (!temporary.isValid()) {
		result.error = u"Could not create temporary export files."_q;
		return result;
	}
	const auto pdfTemporary = temporary.filePath(u"backup.pdf"_q);
	if (!WritePdf(pdfTemporary, html, result.error)) {
		return result;
	}
	auto pdfFile = QFile(pdfTemporary);
	if (!pdfFile.open(QIODevice::ReadOnly)) {
		result.error = pdfFile.errorString();
		return result;
	}
	const auto pdf = pdfFile.readAll();

	const auto write = [&](const QString &suffix, const QByteArray &data) {
		const auto path = target.filePath(baseName + suffix);
		if (!WriteFile(path, data, result.error)) {
			return false;
		}
		result.files.push_back(path);
		return true;
	};
	if (!options.encrypted
		&& ((options.json && !write(u".json"_q, json))
		|| (options.markdown && !write(u".md"_q, markdown))
		|| (options.html && !write(u".html"_q, html))
		|| (options.pdf && !write(u".pdf"_q, pdf)))) {
		return result;
	}
	if (options.zip || options.encrypted) {
		const auto zipTemporary = temporary.filePath(u"backup.zip"_q);
		if (!CreateZip(
				zipTemporary,
				accountId,
				options.dialogId,
				options.messageIds,
				json,
				markdown,
				html,
				pdf,
				result.error)) {
			return result;
		}
		const auto suffix = options.encrypted ? u".intelvault"_q : u".zip"_q;
		const auto archivePath = target.filePath(baseName + suffix);
		if (options.encrypted) {
			if (!EncryptFile(zipTemporary, archivePath, options.passphrase, result.error)) {
				return result;
			}
		} else if (!QFile::copy(zipTemporary, archivePath)) {
			result.error = u"Could not copy the ZIP archive to the export folder."_q;
			return result;
		}
		result.files.push_back(archivePath);
	}
	result.ok = true;
	return result;
}

} // namespace IntelGram::Export
