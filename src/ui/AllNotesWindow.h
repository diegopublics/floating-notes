#pragma once

#include "core/Note.h"
#include "persistence/NoteRepository.h"

#include <QWidget>
#include <QVector>

#include <functional>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTimer;

class AllNotesWindow final : public QWidget
{
public:
    explicit AllNotesWindow(NoteRepository *repository,
                            std::function<void()> notesChangedCallback,
                            QWidget *parent = nullptr);

    void refresh();
    void saveCurrentNoteNow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void selectCurrentListItem();
    void selectNoteByIndex(int index);
    void scheduleSave();
    void saveCurrentNote();
    void archiveOrRestoreCurrentNote();
    void deleteCurrentNote();
    void undoDelete();
    void finalizePendingDelete();
    void insertChecklistLine();
    void importNotes();
    void exportBackup();
    void exportMarkdown();
    void exportText();
    QVector<Note> exportNotes() const;
    void notifyNotesChanged();
    void updateEditor();
    void updateList();
    void updateStatus(const QString &text);

    NoteRepository::NoteListFilter currentFilter() const;

    NoteRepository *m_repository = nullptr;
    std::function<void()> m_notesChangedCallback;
    QVector<Note> m_notes;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_filterCombo = nullptr;
    QListWidget *m_notesList = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_tagsEdit = nullptr;
    QPlainTextEdit *m_bodyEdit = nullptr;
    QPushButton *m_archiveRestoreButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_undoButton = nullptr;
    QPushButton *m_checklistButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_saveTimer = nullptr;
    QTimer *m_deleteUndoTimer = nullptr;
    int m_currentNoteIndex = -1;
    int m_pendingDeleteNoteId = 0;
    bool m_loadingEditor = false;
};
