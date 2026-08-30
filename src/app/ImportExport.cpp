#include "ImportExport.h"

#include "persistence/NoteRepository.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>

namespace {
constexpr auto BackupVersion = 1;
const QString DefaultColor = QStringLiteral("#e8b457");

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

QString safeBaseName(const Note &note, int fallbackIndex)
{
    QString baseName = note.title.trimmed();
    if (baseName.isEmpty()) {
        baseName = QStringLiteral("note-%1").arg(fallbackIndex + 1);
    }

    baseName.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|]+)")), QStringLiteral("-"));
    return baseName.left(80);
}

bool writeFile(const QString &filePath, const QString &content, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        setError(errorMessage, QStringLiteral("Could not write %1.").arg(QDir::toNativeSeparators(filePath)));
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << content;
    return true;
}

QJsonObject noteToJson(const Note &note)
{
    QJsonObject object;
    object.insert(QStringLiteral("title"), note.title);
    object.insert(QStringLiteral("body"), note.body);
    object.insert(QStringLiteral("color"), note.color);
    object.insert(QStringLiteral("tags"), note.tags);
    object.insert(QStringLiteral("updatedAt"), note.updatedAt);
    object.insert(QStringLiteral("archived"), note.archived);
    return object;
}

Note noteFromJson(const QJsonObject &object)
{
    Note note;
    note.title = object.value(QStringLiteral("title")).toString(QStringLiteral("Imported note"));
    note.body = object.value(QStringLiteral("body")).toString();
    note.color = object.value(QStringLiteral("color")).toString(DefaultColor);
    note.tags = object.value(QStringLiteral("tags")).toString();
    note.updatedAt = object.value(QStringLiteral("updatedAt")).toString();
    note.archived = object.value(QStringLiteral("archived")).toBool(false);
    return note;
}
}

bool ImportExport::exportBackup(const QString &filePath, const QVector<Note> &notes, QString *errorMessage)
{
    QJsonArray notesArray;
    for (const Note &note : notes) {
        notesArray.append(noteToJson(note));
    }

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("Floating Notes backup"));
    root.insert(QStringLiteral("version"), BackupVersion);
    root.insert(QStringLiteral("notes"), notesArray);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setError(errorMessage, QStringLiteral("Could not write backup file."));
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ImportExport::exportMarkdown(const QString &directoryPath, const QVector<Note> &notes, QString *errorMessage)
{
    QDir directory(directoryPath);
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        setError(errorMessage, QStringLiteral("Could not create export directory."));
        return false;
    }

    for (int index = 0; index < notes.size(); ++index) {
        const Note &note = notes.at(index);
        QString content = QStringLiteral("# %1\n\n").arg(note.title.trimmed().isEmpty() ? QStringLiteral("Untitled") : note.title.trimmed());
        if (!note.tags.trimmed().isEmpty()) {
            content += QStringLiteral("Tags: %1\n\n").arg(note.tags.trimmed());
        }
        content += note.body;
        if (!content.endsWith(QLatin1Char('\n'))) {
            content += QLatin1Char('\n');
        }

        if (!writeFile(directory.filePath(safeBaseName(note, index) + QStringLiteral(".md")), content, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool ImportExport::exportText(const QString &directoryPath, const QVector<Note> &notes, QString *errorMessage)
{
    QDir directory(directoryPath);
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        setError(errorMessage, QStringLiteral("Could not create export directory."));
        return false;
    }

    for (int index = 0; index < notes.size(); ++index) {
        const Note &note = notes.at(index);
        QString content = note.body;
        if (!content.endsWith(QLatin1Char('\n'))) {
            content += QLatin1Char('\n');
        }

        if (!writeFile(directory.filePath(safeBaseName(note, index) + QStringLiteral(".txt")), content, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool ImportExport::importFiles(const QStringList &filePaths, NoteRepository *repository, QString *errorMessage)
{
    if (repository == nullptr) {
        setError(errorMessage, QStringLiteral("Repository is not available."));
        return false;
    }

    for (const QString &filePath : filePaths) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            setError(errorMessage, QStringLiteral("Could not read %1.").arg(QDir::toNativeSeparators(filePath)));
            return false;
        }

        const QFileInfo fileInfo(file);
        const QString suffix = fileInfo.suffix().toLower();
        if (suffix == QStringLiteral("fnotes")) {
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
            const QJsonArray notes = document.object().value(QStringLiteral("notes")).toArray();
            for (const QJsonValue &value : notes) {
                if (!repository->importNote(noteFromJson(value.toObject()), nullptr, errorMessage)) {
                    return false;
                }
            }
            continue;
        }

        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        Note note;
        note.title = fileInfo.completeBaseName();
        note.body = stream.readAll();
        note.color = DefaultColor;
        if (!repository->importNote(note, nullptr, errorMessage)) {
            return false;
        }
    }

    return true;
}
