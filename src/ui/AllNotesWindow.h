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
class MarkdownEditor;
class QPushButton;
class QTimer;
class AppSettings;

class AllNotesWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit AllNotesWindow(NoteRepository *repository,
                            AppSettings *settings,
                            std::function<void()> notesChangedCallback,
                            QWidget *parent = nullptr);

    void refresh();
    void saveCurrentNoteNow();
    void applySettings();

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
    void undoCurrentNote();
    void redoCurrentNote();
    void finalizePendingDelete();
    void importNotes();
    void exportBackup();
    void exportMarkdown();
    void exportText();
    QVector<Note> exportNotes() const;
    void notifyNotesChanged();
    void updateEditor();
    void updateList();
    void updateStatus(const QString &text);
    void updateHistoryButtons();

    NoteRepository::NoteListFilter currentFilter() const;

    NoteRepository *m_repository = nullptr;
    AppSettings *m_settings = nullptr;
    std::function<void()> m_notesChangedCallback;
    QVector<Note> m_notes;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_filterCombo = nullptr;
    QListWidget *m_notesList = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_tagsEdit = nullptr;
    MarkdownEditor *m_bodyEdit = nullptr;
    QPushButton *m_archiveRestoreButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_undoButton = nullptr;
    QPushButton *m_historyUndoButton = nullptr;
    QPushButton *m_historyRedoButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_saveTimer = nullptr;
    QTimer *m_deleteUndoTimer = nullptr;
    int m_currentNoteIndex = -1;
    int m_pendingDeleteNoteId = 0;
    bool m_loadingEditor = false;
};
