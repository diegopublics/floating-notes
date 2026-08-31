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
constexpr int MaximumHistoryRevisions = 20;

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
    if (!database.transaction()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    QSqlQuery currentQuery(database);
    currentQuery.prepare(QStringLiteral(
        "SELECT title, body, color, tags, history_position "
        "FROM notes WHERE id = :id"));
    currentQuery.bindValue(QStringLiteral(":id"), note.id);
    if (!execute(currentQuery, errorMessage) || !currentQuery.next()) {
        database.rollback();
        setError(errorMessage, QStringLiteral("Note could not be loaded for history."));
        return false;
    }

    const QString body = nonNullString(note.body);
    const QString tags = nonNullString(note.tags);
    const QString currentTitle = currentQuery.value(0).toString();
    const QString currentBody = currentQuery.value(1).toString();
    const QString currentColor = currentQuery.value(2).toString();
    const QString currentTags = currentQuery.value(3).toString();
    int historyPosition = currentQuery.value(4).toInt();
    if (currentTitle == note.title && currentBody == body && currentColor == note.color && currentTags == tags) {
        return database.commit() || (setError(errorMessage, database.lastError().text()), false);
    }

    QSqlQuery historyCountQuery(database);
    historyCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM note_history WHERE note_id = :note_id"));
    historyCountQuery.bindValue(QStringLiteral(":note_id"), note.id);
    if (!execute(historyCountQuery, errorMessage) || !historyCountQuery.next()) {
        database.rollback();
        return false;
    }

    if (historyCountQuery.value(0).toInt() == 0) {
        QSqlQuery initialHistoryQuery(database);
        initialHistoryQuery.prepare(QStringLiteral(
            "INSERT INTO note_history (note_id, revision, title, body, color, tags) "
            "VALUES (:note_id, 0, :title, :body, :color, :tags)"));
        initialHistoryQuery.bindValue(QStringLiteral(":note_id"), note.id);
        initialHistoryQuery.bindValue(QStringLiteral(":title"), currentTitle);
        initialHistoryQuery.bindValue(QStringLiteral(":body"), currentBody);
        initialHistoryQuery.bindValue(QStringLiteral(":color"), currentColor);
        initialHistoryQuery.bindValue(QStringLiteral(":tags"), currentTags);
        if (!execute(initialHistoryQuery, errorMessage)) {
            database.rollback();
            return false;
        }
        historyPosition = 0;
    }

    QSqlQuery deleteRedoQuery(database);
    deleteRedoQuery.prepare(QStringLiteral("DELETE FROM note_history WHERE note_id = :note_id AND revision > :revision"));
    deleteRedoQuery.bindValue(QStringLiteral(":note_id"), note.id);
    deleteRedoQuery.bindValue(QStringLiteral(":revision"), historyPosition);
    if (!execute(deleteRedoQuery, errorMessage)) {
        database.rollback();
        return false;
    }

    QSqlQuery nextRevisionQuery(database);
    nextRevisionQuery.prepare(QStringLiteral("SELECT COALESCE(MAX(revision), -1) + 1 FROM note_history WHERE note_id = :note_id"));
    nextRevisionQuery.bindValue(QStringLiteral(":note_id"), note.id);
    if (!execute(nextRevisionQuery, errorMessage) || !nextRevisionQuery.next()) {
        database.rollback();
        return false;
    }
    historyPosition = nextRevisionQuery.value(0).toInt();

    QSqlQuery insertHistoryQuery(database);
    insertHistoryQuery.prepare(QStringLiteral(
        "INSERT INTO note_history (note_id, revision, title, body, color, tags) "
        "VALUES (:note_id, :revision, :title, :body, :color, :tags)"));
    insertHistoryQuery.bindValue(QStringLiteral(":note_id"), note.id);
    insertHistoryQuery.bindValue(QStringLiteral(":revision"), historyPosition);
    insertHistoryQuery.bindValue(QStringLiteral(":title"), note.title);
    insertHistoryQuery.bindValue(QStringLiteral(":body"), body);
    insertHistoryQuery.bindValue(QStringLiteral(":color"), note.color);
    insertHistoryQuery.bindValue(QStringLiteral(":tags"), tags);
    if (!execute(insertHistoryQuery, errorMessage)) {
        database.rollback();
        return false;
    }

    QSqlQuery updateQuery(database);
    updateQuery.prepare(QStringLiteral(
        "UPDATE notes SET title = :title, body = :body, color = :color, tags = :tags, "
        "history_position = :history_position, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    updateQuery.bindValue(QStringLiteral(":title"), note.title);
    updateQuery.bindValue(QStringLiteral(":body"), body);
    updateQuery.bindValue(QStringLiteral(":color"), note.color);
    updateQuery.bindValue(QStringLiteral(":tags"), tags);
    updateQuery.bindValue(QStringLiteral(":history_position"), historyPosition);
    updateQuery.bindValue(QStringLiteral(":id"), note.id);
    if (!execute(updateQuery, errorMessage)) {
        database.rollback();
        return false;
    }

    QSqlQuery revisionCountQuery(database);
    revisionCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM note_history WHERE note_id = :note_id"));
    revisionCountQuery.bindValue(QStringLiteral(":note_id"), note.id);
    if (!execute(revisionCountQuery, errorMessage) || !revisionCountQuery.next()) {
        database.rollback();
        return false;
    }
    const int excessRevisions = revisionCountQuery.value(0).toInt() - MaximumHistoryRevisions;
    if (excessRevisions > 0) {
        QSqlQuery trimHistoryQuery(database);
        trimHistoryQuery.prepare(QStringLiteral(
            "DELETE FROM note_history WHERE note_id = :note_id AND revision IN ("
            "SELECT revision FROM note_history WHERE note_id = :note_id ORDER BY revision ASC LIMIT :limit)"));
        trimHistoryQuery.bindValue(QStringLiteral(":note_id"), note.id);
        trimHistoryQuery.bindValue(QStringLiteral(":limit"), excessRevisions);
        if (!execute(trimHistoryQuery, errorMessage)) {
            database.rollback();
            return false;
        }
    }

    if (!database.commit()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }
    return true;
}

bool NoteRepository::updateNoteColor(int noteId, const QString &color, QString *errorMessage) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT id, title, body, color, tags, updated_at, archived FROM notes WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), noteId);
    if (!execute(query, errorMessage) || !query.next()) {
        setError(errorMessage, QStringLiteral("Note could not be loaded for color update."));
        return false;
    }
    Note note;
    note.id = query.value(0).toInt();
    note.title = query.value(1).toString();
    note.body = query.value(2).toString();
    note.color = color;
    note.tags = query.value(4).toString();
    note.updatedAt = query.value(5).toString();
    note.archived = query.value(6).toBool();
    return saveNote(note, errorMessage);
}

bool NoteRepository::canUndoNote(int noteId) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT EXISTS(SELECT 1 FROM note_history "
        "WHERE note_id = :note_id AND revision < ("
        "SELECT history_position FROM notes WHERE id = :note_id))"));
    query.bindValue(QStringLiteral(":note_id"), noteId);
    return query.exec() && query.next() && query.value(0).toBool();
}

bool NoteRepository::canRedoNote(int noteId) const
{
    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT EXISTS(SELECT 1 FROM note_history "
        "WHERE note_id = :note_id AND revision > ("
        "SELECT history_position FROM notes WHERE id = :note_id))"));
    query.bindValue(QStringLiteral(":note_id"), noteId);
    return query.exec() && query.next() && query.value(0).toBool();
}

bool NoteRepository::undoNote(int noteId, Note *restoredNote, QString *errorMessage) const
{
    return restoreHistoryRevision(noteId, false, restoredNote, errorMessage);
}

bool NoteRepository::redoNote(int noteId, Note *restoredNote, QString *errorMessage) const
{
    return restoreHistoryRevision(noteId, true, restoredNote, errorMessage);
}

bool NoteRepository::restoreHistoryRevision(int noteId, bool redo, Note *restoredNote, QString *errorMessage) const
{
    if (restoredNote == nullptr) {
        setError(errorMessage, QStringLiteral("Restored note output is null."));
        return false;
    }

    QSqlDatabase database = QSqlDatabase::database(m_connectionName);
    if (!database.transaction()) {
        setError(errorMessage, database.lastError().text());
        return false;
    }

    QSqlQuery positionQuery(database);
    positionQuery.prepare(QStringLiteral("SELECT history_position, archived FROM notes WHERE id = :id"));
    positionQuery.bindValue(QStringLiteral(":id"), noteId);
    if (!execute(positionQuery, errorMessage) || !positionQuery.next()) {
        database.rollback();
        setError(errorMessage, QStringLiteral("Note history is unavailable."));
        return false;
    }
    const int currentPosition = positionQuery.value(0).toInt();
    const bool archived = positionQuery.value(1).toBool();

    QSqlQuery revisionQuery(database);
    revisionQuery.prepare(QStringLiteral(
        "SELECT revision, title, body, color, tags FROM note_history "
        "WHERE note_id = :note_id AND revision %1 :position "
        "ORDER BY revision %2 LIMIT 1")
            .arg(redo ? QStringLiteral(">") : QStringLiteral("<"),
                 redo ? QStringLiteral("ASC") : QStringLiteral("DESC")));
    revisionQuery.bindValue(QStringLiteral(":note_id"), noteId);
    revisionQuery.bindValue(QStringLiteral(":position"), currentPosition);
    if (!execute(revisionQuery, errorMessage) || !revisionQuery.next()) {
        database.rollback();
        setError(errorMessage, redo ? QStringLiteral("No newer revision is available.") : QStringLiteral("No older revision is available."));
        return false;
    }

    const int revision = revisionQuery.value(0).toInt();
    QSqlQuery updateQuery(database);
    updateQuery.prepare(QStringLiteral(
        "UPDATE notes SET title = :title, body = :body, color = :color, tags = :tags, "
        "history_position = :position, updated_at = CURRENT_TIMESTAMP WHERE id = :id"));
    updateQuery.bindValue(QStringLiteral(":title"), revisionQuery.value(1));
    updateQuery.bindValue(QStringLiteral(":body"), revisionQuery.value(2));
    updateQuery.bindValue(QStringLiteral(":color"), revisionQuery.value(3));
    updateQuery.bindValue(QStringLiteral(":tags"), revisionQuery.value(4));
    updateQuery.bindValue(QStringLiteral(":position"), revision);
    updateQuery.bindValue(QStringLiteral(":id"), noteId);
    if (!execute(updateQuery, errorMessage) || !database.commit()) {
        if (errorMessage != nullptr && errorMessage->isEmpty()) {
            *errorMessage = database.lastError().text();
        }
        database.rollback();
        return false;
    }

    restoredNote->id = noteId;
    restoredNote->title = revisionQuery.value(1).toString();
    restoredNote->body = revisionQuery.value(2).toString();
    restoredNote->color = revisionQuery.value(3).toString();
    restoredNote->tags = revisionQuery.value(4).toString();
    restoredNote->archived = archived;
    return true;
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
        "history_position INTEGER NOT NULL DEFAULT 0,"
        "sort_order INTEGER NOT NULL DEFAULT 0,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "deleted_at TEXT"
        ")"))) {
        setError(errorMessage, query.lastError().text());
        return false;
    }

    if (!query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS note_history ("
        "note_id INTEGER NOT NULL,"
        "revision INTEGER NOT NULL,"
        "title TEXT NOT NULL,"
        "body TEXT NOT NULL,"
        "color TEXT NOT NULL,"
        "tags TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (note_id, revision))"))) {
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
                        errorMessage)
        && ensureColumn(QStringLiteral("notes"),
                        QStringLiteral("history_position"),
                        QStringLiteral("history_position INTEGER NOT NULL DEFAULT 0"),
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
