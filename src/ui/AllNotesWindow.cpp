#include "AllNotesWindow.h"

#include "app/ImportExport.h"
#include "core/NoteBodyFormat.h"
#include "MarkdownEditor.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace {
constexpr int SaveDelayMs = 250;
constexpr int UndoDelayMs = 7000;

QString previewText(const Note &note)
{
    const QString body = NoteBodyFormat::toPlainText(note.body).simplified();
    if (body.isEmpty()) {
        return note.archived ? QStringLiteral("Archived") : QStringLiteral("Active");
    }

    return body.left(80);
}
}

AllNotesWindow::AllNotesWindow(NoteRepository *repository,
                               std::function<void()> notesChangedCallback,
                               QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_notesChangedCallback(std::move(notesChangedCallback))
{
    setWindowTitle("All Notes");
    setWindowFlag(Qt::Window, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    resize(860, 520);

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(SaveDelayMs);
    connect(m_saveTimer, &QTimer::timeout, this, [this] {
        saveCurrentNote();
    });

    m_deleteUndoTimer = new QTimer(this);
    m_deleteUndoTimer->setSingleShot(true);
    m_deleteUndoTimer->setInterval(UndoDelayMs);
    connect(m_deleteUndoTimer, &QTimer::timeout, this, [this] {
        finalizePendingDelete();
    });

    buildUi();
    refresh();
}

void AllNotesWindow::refresh()
{
    saveCurrentNote();

    QString errorMessage;
    m_notes = m_repository->loadNotes(currentFilter(), m_searchEdit->text(), &errorMessage);
    if (!errorMessage.isEmpty()) {
        updateStatus(QStringLiteral("Load error: %1").arg(errorMessage));
    }

    updateList();
    m_currentNoteIndex = -1;
    selectNoteByIndex(m_notes.isEmpty() ? -1 : 0);
}

void AllNotesWindow::saveCurrentNoteNow()
{
    saveCurrentNote();
}

void AllNotesWindow::closeEvent(QCloseEvent *event)
{
    saveCurrentNote();
    hide();
    event->ignore();
}

void AllNotesWindow::buildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 14, 14, 14);
    rootLayout->setSpacing(10);

    auto *topLayout = new QHBoxLayout;
    auto *title = new QLabel("All Notes", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search notes or tags");
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this] {
        refresh();
    });

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem("All", static_cast<int>(NoteRepository::NoteListFilter::All));
    m_filterCombo->addItem("Active", static_cast<int>(NoteRepository::NoteListFilter::Active));
    m_filterCombo->addItem("Archived", static_cast<int>(NoteRepository::NoteListFilter::Archived));
    m_filterCombo->setCurrentIndex(1);
    connect(m_filterCombo, &QComboBox::currentIndexChanged, this, [this] {
        refresh();
    });

    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(m_searchEdit, 1);
    topLayout->addWidget(m_filterCombo);
    rootLayout->addLayout(topLayout);

    auto *toolsLayout = new QHBoxLayout;
    auto *importButton = new QPushButton("Import", this);
    connect(importButton, &QPushButton::clicked, this, [this] {
        importNotes();
    });
    auto *backupButton = new QPushButton("Backup", this);
    connect(backupButton, &QPushButton::clicked, this, [this] {
        exportBackup();
    });
    auto *markdownButton = new QPushButton("Markdown", this);
    connect(markdownButton, &QPushButton::clicked, this, [this] {
        exportMarkdown();
    });
    auto *textButton = new QPushButton("Text", this);
    connect(textButton, &QPushButton::clicked, this, [this] {
        exportText();
    });

    toolsLayout->addWidget(importButton);
    toolsLayout->addWidget(backupButton);
    toolsLayout->addWidget(markdownButton);
    toolsLayout->addWidget(textButton);
    toolsLayout->addStretch();
    rootLayout->addLayout(toolsLayout);

    auto *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);

    m_notesList = new QListWidget(this);
    m_notesList->setMinimumWidth(280);
    connect(m_notesList, &QListWidget::currentRowChanged, this, [this] {
        selectCurrentListItem();
    });
    contentLayout->addWidget(m_notesList, 0);

    auto *editorLayout = new QVBoxLayout;
    editorLayout->setSpacing(8);

    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Title");
    connect(m_titleEdit, &QLineEdit::textChanged, this, [this] {
        scheduleSave();
    });
    editorLayout->addWidget(m_titleEdit);

    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setPlaceholderText("Tags");
    connect(m_tagsEdit, &QLineEdit::textChanged, this, [this] {
        scheduleSave();
    });
    editorLayout->addWidget(m_tagsEdit);

    m_bodyEdit = new MarkdownEditor(this);
    m_bodyEdit->setPlaceholderText("Write a note...");
    connect(m_bodyEdit, &MarkdownEditor::textChanged, this, [this] {
        scheduleSave();
    });
    editorLayout->addWidget(m_bodyEdit, 1);

    auto *actionsLayout = new QHBoxLayout;
    m_checklistButton = new QPushButton("Checklist", this);
    connect(m_checklistButton, &QPushButton::clicked, this, [this] {
        insertChecklistLine();
    });

    m_historyUndoButton = new QPushButton("Undo", this);
    m_historyUndoButton->setToolTip("Restore the previous saved version");
    connect(m_historyUndoButton, &QPushButton::clicked, this, &AllNotesWindow::undoCurrentNote);

    m_historyRedoButton = new QPushButton("Redo", this);
    m_historyRedoButton->setToolTip("Restore the next saved version");
    connect(m_historyRedoButton, &QPushButton::clicked, this, &AllNotesWindow::redoCurrentNote);

    m_archiveRestoreButton = new QPushButton("Archive", this);
    connect(m_archiveRestoreButton, &QPushButton::clicked, this, [this] {
        archiveOrRestoreCurrentNote();
    });

    m_deleteButton = new QPushButton("Delete", this);
    connect(m_deleteButton, &QPushButton::clicked, this, [this] {
        deleteCurrentNote();
    });

    m_undoButton = new QPushButton("Undo delete", this);
    m_undoButton->hide();
    connect(m_undoButton, &QPushButton::clicked, this, [this] {
        undoDelete();
    });

    m_statusLabel = new QLabel(this);
    actionsLayout->addWidget(m_checklistButton);
    actionsLayout->addWidget(m_historyUndoButton);
    actionsLayout->addWidget(m_historyRedoButton);
    actionsLayout->addWidget(m_archiveRestoreButton);
    actionsLayout->addWidget(m_deleteButton);
    actionsLayout->addWidget(m_undoButton);
    actionsLayout->addStretch();
    actionsLayout->addWidget(m_statusLabel);
    editorLayout->addLayout(actionsLayout);

    contentLayout->addLayout(editorLayout, 1);
    rootLayout->addLayout(contentLayout, 1);
}

void AllNotesWindow::selectCurrentListItem()
{
    selectNoteByIndex(m_notesList->currentRow());
}

void AllNotesWindow::selectNoteByIndex(int index)
{
    saveCurrentNote();
    m_currentNoteIndex = (index >= 0 && index < m_notes.size()) ? index : -1;
    updateEditor();
}

void AllNotesWindow::scheduleSave()
{
    if (m_loadingEditor || m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    m_saveTimer->start();
    updateStatus(QStringLiteral("Editing..."));
}

void AllNotesWindow::saveCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    Note &note = m_notes[m_currentNoteIndex];
    note.title = m_titleEdit->text().trimmed();
    if (note.title.isEmpty()) {
        note.title = QStringLiteral("Untitled");
    }
    note.tags = m_tagsEdit->text().trimmed();
    note.body = m_bodyEdit->storage();

    QString errorMessage;
    if (!m_repository->saveNote(note, &errorMessage)) {
        updateStatus(QStringLiteral("Save error: %1").arg(errorMessage));
        return;
    }

    updateStatus(QStringLiteral("Saved"));
    updateHistoryButtons();
    updateList();
    notifyNotesChanged();
}

void AllNotesWindow::archiveOrRestoreCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    saveCurrentNote();

    Note &note = m_notes[m_currentNoteIndex];
    QString errorMessage;
    const bool ok = note.archived
        ? m_repository->restoreNote(note.id, &errorMessage)
        : m_repository->archiveNote(note.id, &errorMessage);

    if (!ok) {
        updateStatus(QStringLiteral("State error: %1").arg(errorMessage));
        return;
    }

    updateStatus(note.archived ? QStringLiteral("Restored") : QStringLiteral("Archived"));
    notifyNotesChanged();
    refresh();
}

void AllNotesWindow::deleteCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    finalizePendingDelete();
    saveCurrentNote();

    const int deletedIndex = m_currentNoteIndex;
    m_pendingDeleteNoteId = m_notes.at(deletedIndex).id;

    QString errorMessage;
    if (!m_repository->markNoteDeleted(m_pendingDeleteNoteId, &errorMessage)) {
        updateStatus(QStringLiteral("Delete error: %1").arg(errorMessage));
        m_pendingDeleteNoteId = 0;
        return;
    }

    m_notes.removeAt(deletedIndex);
    updateList();
    selectNoteByIndex(std::min(deletedIndex, static_cast<int>(m_notes.size()) - 1));
    m_undoButton->show();
    m_deleteUndoTimer->start();
    updateStatus(QStringLiteral("Deleted"));
    notifyNotesChanged();
}

void AllNotesWindow::undoDelete()
{
    if (m_pendingDeleteNoteId == 0) {
        return;
    }

    const int noteId = m_pendingDeleteNoteId;
    m_pendingDeleteNoteId = 0;
    m_deleteUndoTimer->stop();
    m_undoButton->hide();

    QString errorMessage;
    if (!m_repository->restoreDeletedNote(noteId, &errorMessage)) {
        updateStatus(QStringLiteral("Undo error: %1").arg(errorMessage));
        return;
    }

    updateStatus(QStringLiteral("Restored"));
    notifyNotesChanged();
    refresh();
}

void AllNotesWindow::undoCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }
    m_saveTimer->stop();
    saveCurrentNote();
    Note restoredNote;
    QString errorMessage;
    if (!m_repository->undoNote(m_notes.at(m_currentNoteIndex).id, &restoredNote, &errorMessage)) {
        updateStatus(QStringLiteral("Undo error: %1").arg(errorMessage));
        return;
    }
    m_notes[m_currentNoteIndex] = restoredNote;
    updateEditor();
    updateList();
    updateStatus(QStringLiteral("Previous version restored"));
    notifyNotesChanged();
}

void AllNotesWindow::redoCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }
    m_saveTimer->stop();
    saveCurrentNote();
    Note restoredNote;
    QString errorMessage;
    if (!m_repository->redoNote(m_notes.at(m_currentNoteIndex).id, &restoredNote, &errorMessage)) {
        updateStatus(QStringLiteral("Redo error: %1").arg(errorMessage));
        return;
    }
    m_notes[m_currentNoteIndex] = restoredNote;
    updateEditor();
    updateList();
    updateStatus(QStringLiteral("Next version restored"));
    notifyNotesChanged();
}

void AllNotesWindow::finalizePendingDelete()
{
    if (m_pendingDeleteNoteId == 0) {
        return;
    }

    const int noteId = m_pendingDeleteNoteId;
    m_pendingDeleteNoteId = 0;
    m_deleteUndoTimer->stop();
    m_undoButton->hide();

    QString errorMessage;
    if (!m_repository->deleteNotePermanently(noteId, &errorMessage)) {
        updateStatus(QStringLiteral("Delete cleanup error: %1").arg(errorMessage));
    }
}

void AllNotesWindow::insertChecklistLine()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    m_bodyEdit->insertChecklistItem();
    scheduleSave();
}

void AllNotesWindow::importNotes()
{
    const QStringList filePaths = QFileDialog::getOpenFileNames(this,
                                                                "Import Notes",
                                                                QString(),
                                                                "Notes (*.fnotes *.md *.txt)");
    if (filePaths.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::importFiles(filePaths, m_repository, &errorMessage)) {
        updateStatus(QStringLiteral("Import error: %1").arg(errorMessage));
        return;
    }

    updateStatus(QStringLiteral("Imported"));
    notifyNotesChanged();
    refresh();
}

void AllNotesWindow::exportBackup()
{
    const QString filePath = QFileDialog::getSaveFileName(this, "Export Backup", QStringLiteral("floating-notes.fnotes"), "Floating Notes (*.fnotes)");
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::exportBackup(filePath, exportNotes(), &errorMessage)) {
        updateStatus(QStringLiteral("Export error: %1").arg(errorMessage));
        return;
    }
    updateStatus(QStringLiteral("Backup exported"));
}

void AllNotesWindow::exportMarkdown()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(this, "Export Markdown");
    if (directoryPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::exportMarkdown(directoryPath, exportNotes(), &errorMessage)) {
        updateStatus(QStringLiteral("Export error: %1").arg(errorMessage));
        return;
    }
    updateStatus(QStringLiteral("Markdown exported"));
}

void AllNotesWindow::exportText()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(this, "Export Text");
    if (directoryPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::exportText(directoryPath, exportNotes(), &errorMessage)) {
        updateStatus(QStringLiteral("Export error: %1").arg(errorMessage));
        return;
    }
    updateStatus(QStringLiteral("Text exported"));
}

QVector<Note> AllNotesWindow::exportNotes() const
{
    QString errorMessage;
    QVector<Note> notes = m_repository->loadNotes(NoteRepository::NoteListFilter::All, {}, &errorMessage);
    return notes;
}

void AllNotesWindow::notifyNotesChanged()
{
    if (m_notesChangedCallback) {
        m_notesChangedCallback();
    }
}

void AllNotesWindow::updateEditor()
{
    m_loadingEditor = true;

    const bool hasNote = m_currentNoteIndex >= 0 && m_currentNoteIndex < m_notes.size();
    m_titleEdit->setEnabled(hasNote);
    m_tagsEdit->setEnabled(hasNote);
    m_bodyEdit->setEnabled(hasNote);
    m_checklistButton->setEnabled(hasNote);
    updateHistoryButtons();
    m_archiveRestoreButton->setEnabled(hasNote);
    m_deleteButton->setEnabled(hasNote);

    if (hasNote) {
        const Note &note = m_notes.at(m_currentNoteIndex);
        m_titleEdit->setText(note.title);
        m_tagsEdit->setText(note.tags);
        m_bodyEdit->setMarkdown(note.body);
        m_archiveRestoreButton->setText(note.archived ? QStringLiteral("Restore") : QStringLiteral("Archive"));
    } else {
        m_titleEdit->clear();
        m_tagsEdit->clear();
        m_bodyEdit->setMarkdown({});
        m_archiveRestoreButton->setText(QStringLiteral("Archive"));
    }

    m_loadingEditor = false;
}

void AllNotesWindow::updateHistoryButtons()
{
    const bool hasNote = m_currentNoteIndex >= 0 && m_currentNoteIndex < m_notes.size();
    m_historyUndoButton->setEnabled(hasNote && m_repository->canUndoNote(m_notes.at(m_currentNoteIndex).id));
    m_historyRedoButton->setEnabled(hasNote && m_repository->canRedoNote(m_notes.at(m_currentNoteIndex).id));
}

void AllNotesWindow::updateList()
{
    const int previousRow = m_notesList->currentRow();
    m_notesList->blockSignals(true);
    m_notesList->clear();

    for (const Note &note : m_notes) {
        QString text = note.title + QStringLiteral("\n") + previewText(note);
        if (!note.tags.trimmed().isEmpty()) {
            text += QStringLiteral("\n") + note.tags.trimmed();
        }
        auto *item = new QListWidgetItem(text);
        item->setData(Qt::DecorationRole, QColor(note.color));
        item->setData(Qt::UserRole, note.id);
        item->setToolTip(note.updatedAt.isEmpty() ? QString() : QStringLiteral("Updated: %1").arg(note.updatedAt));
        m_notesList->addItem(item);
    }

    if (!m_notes.isEmpty()) {
        m_notesList->setCurrentRow(std::clamp(previousRow, 0, static_cast<int>(m_notes.size()) - 1));
    }

    m_notesList->blockSignals(false);
}

void AllNotesWindow::updateStatus(const QString &text)
{
    m_statusLabel->setText(text);
}

NoteRepository::NoteListFilter AllNotesWindow::currentFilter() const
{
    return static_cast<NoteRepository::NoteListFilter>(m_filterCombo->currentData().toInt());
}
