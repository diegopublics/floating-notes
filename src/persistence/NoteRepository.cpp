#include "NoteRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace {
constexpr auto DriverName = "QSQLITE";
constexpr auto DatabaseFileName = "notes.sqlite";

bool execute(QSqlQuery &query, QString *errorMessage)
{
    if (query.exec()) {
        return true;
    }

    if (errorMessage != nullptr) {
        *errorMessage = query.lastError().text();
    }
    return false;
}

QString nonNullString(const QString &value)
{
    return value.isNull() ? QStringLiteral("") : value;
}
}

NoteRepository::NoteRepository()
    : m_connectionName(QStringLiteral("floating_notes_connection"))
{
}

NoteRepository::~NoteRepository()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName, false);
        if (database.isValid()) {
            database.close();
        }
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool NoteRepository::open(QString *errorMessage)
{
    if (!QSqlDatabase::isDriverAvailable(DriverName)) {
        setError(errorMessage, QStringLiteral("SQLite driver is not available."));
        return false;
    }

    QSqlDatabase database = QSqlDatabase::contains(m_connectionName)
        ? QSqlDatabase::database(m_connectionName)
        : QSqlDatabase::addDatabase(DriverName, m_connectionName);

    const QString path = databasePath();
    if (path.isEmpty()) {
        setError(errorMessage, QStringLiteral("Could not resolve the application data directory."));
        return false;
    }

    const QFileInfo fileInfo(path);
    QDir directory = fileInfo.dir();
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        setError(errorMessage, QStringLiteral("Could not create the application data directory."));
        return false;
    }

    database.setDatabaseName(path);
    if (!database.open()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    return ensureSchema(errorMessage) && seedIfEmpty(errorMessage);
}

QVector<Note> NoteRepository::loadNotes(NoteListFilter filter, const QString &searchText, QString *errorMessage) const
{
    QVector<Note> notes;
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);

    QString sql = QStringLiteral(
        "SELECT id, title, body, color, tags, updated_at, archived "
        "FROM notes "
        "WHERE deleted_at IS NULL ");

    if (filter == NoteListFilter::Active) {
        sql += QStringLiteral("AND archived = 0 ");
    } else if (filter == NoteListFilter::Archived) {
        sql += QStringLiteral("AND archived = 1 ");
    }

    const QString trimmedSearch = searchText.trimmed();
    if (!trimmedSearch.isEmpty()) {
        sql += QStringLiteral("AND (title LIKE :search OR body LIKE :search OR tags LIKE :search) ");
    }

    sql += QStringLiteral("ORDER BY sort_order, id");

    query.prepare(sql);
    if (!trimmedSearch.isEmpty()) {
        query.bindValue(QStringLiteral(":search"), QStringLiteral("%%1%").arg(trimmedSearch));
    }

    if (!execute(query, errorMessage)) {
        setError(errorMessage, query.lastError().text());
        return notes;
    }

    while (query.next()) {
        Note note;
        note.id = query.value(0).toInt();
        note.title = query.value(1).toString();
        note.body = query.value(2).toString();
        note.color = query.value(3).toString();
        note.tags = query.value(4).toString();
        note.updatedAt = query.value(5).toString();
        note.archived = query.value(6).toBool();
        notes.append(note);
    }

    return notes;
}

bool NoteRepository::createNote(Note *createdNote, QString *errorMessage) const
{
    if (createdNote == nullptr) {
        setError(errorMessage, QStringLiteral("Created note output is null."));
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO notes (title, body, color, tags, archived, sort_order) "
        "VALUES (:title, :body, :color, :tags, 0, "
        "COALESCE((SELECT MAX(sort_order) + 1 FROM notes), 0))"));
    query.bindValue(QStringLiteral(":title"), QStringLiteral("Untitled"));
    query.bindValue(QStringLiteral(":body"), QStringLiteral(""));
    query.bindValue(QStringLiteral(":color"), QStringLiteral("#e8b457"));
    query.bindValue(QStringLiteral(":tags"), QStringLiteral(""));

    if (!execute(query, errorMessage)) {
        return false;
    }

    const int noteId = query.lastInsertId().toInt();
    QSqlQuery selectQuery(database);
    selectQuery.prepare(QStringLiteral(
        "SELECT id, title, body, color, tags, updated_at, archived "
        "FROM notes "
        "WHERE id = :id"));
    selectQuery.bindValue(QStringLiteral(":id"), noteId);

    if (!execute(selectQuery, errorMessage) || !selectQuery.next()) {
        setError(errorMessage, QStringLiteral("Created note could not be loaded."));
        return false;
    }

    createdNote->id = selectQuery.value(0).toInt();
    createdNote->title = selectQuery.value(1).toString();
    createdNote->body = selectQuery.value(2).toString();
    createdNote->color = selectQuery.value(3).toString();
    createdNote->tags = selectQuery.value(4).toString();
    createdNote->updatedAt = selectQuery.value(5).toString();
    createdNote->archived = selectQuery.value(6).toBool();
    return true;
}

bool NoteRepository::importNote(const Note &note, Note *createdNote, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "INSERT INTO notes (title, body, color, tags, archived, sort_order) "
        "VALUES (:title, :body, :color, :tags, :archived, "
        "COALESCE((SELECT MAX(sort_order) + 1 FROM notes), 0))"));
    query.bindValue(QStringLiteral(":title"), note.title.trimmed().isEmpty() ? QStringLiteral("Imported note") : note.title.trimmed());
    query.bindValue(QStringLiteral(":body"), note.body.isNull() ? QStringLiteral("") : note.body);
    query.bindValue(QStringLiteral(":color"), note.color.trimmed().isEmpty() ? QStringLiteral("#e8b457") : note.color.trimmed());
    query.bindValue(QStringLiteral(":tags"), nonNullString(note.tags).trimmed());
    query.bindValue(QStringLiteral(":archived"), note.archived ? 1 : 0);

    if (!execute(query, errorMessage)) {
        return false;
    }

    if (createdNote == nullptr) {
        return true;
    }

    const int noteId = query.lastInsertId().toInt();
    QSqlQuery selectQuery(database);
    selectQuery.prepare(QStringLiteral(
        "SELECT id, title, body, color, tags, updated_at, archived "
        "FROM notes "
        "WHERE id = :id"));
    selectQuery.bindValue(QStringLiteral(":id"), noteId);

    if (!execute(selectQuery, errorMessage) || !selectQuery.next()) {
        setError(errorMessage, QStringLiteral("Imported note could not be loaded."));
        return false;
    }

    createdNote->id = selectQuery.value(0).toInt();
    createdNote->title = selectQuery.value(1).toString();
    createdNote->body = selectQuery.value(2).toString();
    createdNote->color = selectQuery.value(3).toString();
    createdNote->tags = selectQuery.value(4).toString();
    createdNote->updatedAt = selectQuery.value(5).toString();
    createdNote->archived = selectQuery.value(6).toBool();
    return true;
}

bool NoteRepository::archiveNote(int noteId, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "UPDATE notes "
        "SET archived = 1, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), noteId);
    return execute(query, errorMessage);
}

bool NoteRepository::restoreNote(int noteId, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "UPDATE notes "
        "SET archived = 0, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), noteId);
    return execute(query, errorMessage);
}

bool NoteRepository::markNoteDeleted(int noteId, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "UPDATE notes "
        "SET deleted_at = CURRENT_TIMESTAMP, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), noteId);
    return execute(query, errorMessage);
}

bool NoteRepository::restoreDeletedNote(int noteId, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "UPDATE notes "
        "SET deleted_at = NULL, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), noteId);
    return execute(query, errorMessage);
}

bool NoteRepository::deleteNotePermanently(int noteId, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral("DELETE FROM notes WHERE id = :id AND deleted_at IS NOT NULL"));
    query.bindValue(QStringLiteral(":id"), noteId);
    return execute(query, errorMessage);
}

bool NoteRepository::saveNote(const Note &note, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "UPDATE notes "
        "SET title = :title, body = :body, tags = :tags, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":title"), note.title);
    query.bindValue(QStringLiteral(":body"), note.body.isNull() ? QStringLiteral("") : note.body);
    query.bindValue(QStringLiteral(":tags"), nonNullString(note.tags));
    query.bindValue(QStringLiteral(":id"), note.id);
    return execute(query, errorMessage);
}

bool NoteRepository::updateNoteColor(int noteId, const QString &color, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "UPDATE notes "
        "SET color = :color, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":color"), color);
    query.bindValue(QStringLiteral(":id"), noteId);
    return execute(query, errorMessage);
}

bool NoteRepository::ensureSchema(QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "body TEXT NOT NULL,"
        "color TEXT NOT NULL,"
        "tags TEXT NOT NULL DEFAULT '',"
        "archived INTEGER NOT NULL DEFAULT 0,"
        "sort_order INTEGER NOT NULL DEFAULT 0,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "deleted_at TEXT"
        ")"))) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    return ensureColumn(QStringLiteral("notes"),
                        QStringLiteral("tags"),
                        QStringLiteral("tags TEXT NOT NULL DEFAULT ''"),
                        errorMessage)
        && ensureColumn(QStringLiteral("notes"),
                        QStringLiteral("archived"),
                        QStringLiteral("archived INTEGER NOT NULL DEFAULT 0"),
                        errorMessage)
        && ensureColumn(QStringLiteral("notes"),
                        QStringLiteral("deleted_at"),
                        QStringLiteral("deleted_at TEXT"),
                        errorMessage);
}

bool NoteRepository::ensureColumn(const QString &tableName,
                                  const QString &columnName,
                                  const QString &definition,
                                  QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tableName))) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    while (query.next()) {
        if (query.value(1).toString() == columnName) {
            return true;
        }
    }

    QSqlQuery alterQuery(database);
    return alterQuery.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2").arg(tableName, definition))
        || (setError(errorMessage, alterQuery.lastError().text()), false);
}

bool NoteRepository::seedIfEmpty(QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery countQuery(database);
    if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM notes")) || !countQuery.next()) {
        setError(errorMessage, countQuery.lastError().text());
        return false;
    }

    if (countQuery.value(0).toInt() > 0) {
        return true;
    }

    const QVector<Note> seedNotes = {
        {0, QStringLiteral("Today"), QStringLiteral("Write your first floating note."), QStringLiteral("#e8b457"), QStringLiteral("#today"), {}, false},
        {0, QStringLiteral("Build"), QStringLiteral("Qt Widgets, CMake, SQLite."), QStringLiteral("#79a7d3"), QStringLiteral("#build"), {}, false},
        {0, QStringLiteral("Ideas"), QStringLiteral("Keep the app small and local."), QStringLiteral("#8dbf76"), QStringLiteral("#ideas"), {}, false},
    };

    QSqlQuery insertQuery(database);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO notes (title, body, color, tags, archived, sort_order) "
        "VALUES (:title, :body, :color, :tags, 0, :sort_order)"));

    for (qsizetype index = 0; index < seedNotes.size(); ++index) {
        const Note &note = seedNotes.at(index);
        insertQuery.bindValue(QStringLiteral(":title"), note.title);
        insertQuery.bindValue(QStringLiteral(":body"), note.body);
        insertQuery.bindValue(QStringLiteral(":color"), note.color);
        insertQuery.bindValue(QStringLiteral(":tags"), nonNullString(note.tags));
        insertQuery.bindValue(QStringLiteral(":sort_order"), index);

        if (!execute(insertQuery, errorMessage)) {
            return false;
        }
    }

    return true;
}

QString NoteRepository::databasePath() const
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (directory.isEmpty()) {
        directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }

    if (directory.isEmpty()) {
        return {};
    }

    return QDir(directory).filePath(DatabaseFileName);
}

void NoteRepository::setError(QString *errorMessage, const QString &message) const
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
