#pragma once

#include "core/Note.h"

#include <QString>
#include <QStringList>
#include <QVector>

class NoteRepository;

namespace ImportExport {
bool exportBackup(const QString &filePath, const QVector<Note> &notes, QString *errorMessage = nullptr);
bool exportMarkdown(const QString &directoryPath, const QVector<Note> &notes, QString *errorMessage = nullptr);
bool exportText(const QString &directoryPath, const QVector<Note> &notes, QString *errorMessage = nullptr);
bool importFiles(const QStringList &filePaths, NoteRepository *repository, QString *errorMessage = nullptr);
}
