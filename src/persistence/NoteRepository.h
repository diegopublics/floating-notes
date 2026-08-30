#pragma once

#include "core/Note.h"

#include <QString>
#include <QVector>

class NoteRepository
{
public:
    enum class NoteListFilter {
        All,
        Active,
        Archived,
    };

    NoteRepository();
    ~NoteRepository();

    bool open(QString *errorMessage = nullptr);
    bool createNote(Note *createdNote, QString *errorMessage = nullptr) const;
    bool importNote(const Note &note, Note *createdNote = nullptr, QString *errorMessage = nullptr) const;
    QVector<Note> loadNotes(NoteListFilter filter = NoteListFilter::Active,
                             const QString &searchText = {},
                             QString *errorMessage = nullptr) const;
    bool archiveNote(int noteId, QString *errorMessage = nullptr) const;
    bool restoreNote(int noteId, QString *errorMessage = nullptr) const;
    bool markNoteDeleted(int noteId, QString *errorMessage = nullptr) const;
    bool restoreDeletedNote(int noteId, QString *errorMessage = nullptr) const;
    bool deleteNotePermanently(int noteId, QString *errorMessage = nullptr) const;
    bool saveNote(const Note &note, QString *errorMessage = nullptr) const;
    bool updateNoteColor(int noteId, const QString &color, QString *errorMessage = nullptr) const;

private:
    bool ensureSchema(QString *errorMessage) const;
    bool ensureColumn(const QString &tableName, const QString &columnName, const QString &definition, QString *errorMessage) const;
    bool seedIfEmpty(QString *errorMessage) const;
    QString databasePath() const;
    void setError(QString *errorMessage, const QString &message) const;

    QString m_connectionName;
};
