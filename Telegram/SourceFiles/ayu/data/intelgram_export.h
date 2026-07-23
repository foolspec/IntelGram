// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ayu/data/intelgram_vault.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace IntelGram::Export {

struct Options {
	bool html = true;
	bool json = true;
	bool markdown = true;
	bool pdf = true;
	bool zip = true;
	bool encrypted = false;
	Vault::ID dialogId = 0;
	std::vector<int> messageIds;
	QString passphrase;
};

struct Result {
	bool ok = false;
	QString error;
	QStringList files;
};

[[nodiscard]] Result Create(Vault::ID accountId, const QString &directory, const Options &options);

} // namespace IntelGram::Export
