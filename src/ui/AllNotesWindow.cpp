#include "AllNotesWindow.h"

#include "app/AppSettings.h"
#include "app/ImportExport.h"
#include "core/NoteBodyFormat.h"
#include "AppIcons.h"
#include "MarkdownEditor.h"

#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QStyleHints>

#include <algorithm>
#include <utility>

namespace {
constexpr int SaveDelayMs = 250;
constexpr int UndoDelayMs = 7000;

QColor editorInkColor(const QString &paperColor)
{
    const QColor color(paperColor);
    const int luminance = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    return luminance > 155 ? QColor(QStringLiteral("#38252d")) : QColor(QStringLiteral("#fff8f1"));
}

bool usesDarkTheme(const AppSettings *settings)
{
    if (settings->theme() == AppSettings::Theme::Dark) {
        return true;
    }
    if (settings->theme() == AppSettings::Theme::Light) {
        return false;
    }
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QString richTextEditorStyle(const QColor &ink)
{
    return QStringLiteral(
               "QWidget#noteBodyEditor { background: transparent; } "
               "QPlainTextEdit, QTextEdit { background: transparent; border: 0; color: %1; "
               "selection-background-color: rgba(74,45,57,185); selection-color: #fff8f1; padding: 9px 8px; } "
               "QFrame#markdownToolbar { background: rgba(61,36,47,16); border-bottom: 1px solid rgba(61,36,47,38); } "
               "QToolButton { background: transparent; border: 0; border-radius: 4px; color: %1; padding: 1px; font-size: 10px; } "
               "QToolButton:hover, QToolButton:checked { background: rgba(61,36,47,48); }")
        .arg(ink.name());
}

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
                               AppSettings *settings,
                               std::function<void()> notesChangedCallback,
                               QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_settings(settings)
    , m_notesChangedCallback(std::move(notesChangedCallback))
{
    setWindowTitle(tr("All Notes"));
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
        updateStatus(tr("Load error: %1").arg(errorMessage));
    }

    updateList();
    m_currentNoteIndex = -1;
    selectNoteByIndex(m_notes.isEmpty() ? -1 : 0);
}

void AllNotesWindow::saveCurrentNoteNow()
{
    saveCurrentNote();
}

void AllNotesWindow::applySettings()
{
    updateEditor();
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
    auto *title = new QLabel(tr("All Notes"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setWeight(QFont::DemiBold);
    title->setFont(titleFont);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search notes or tags"));
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this] {
        refresh();
    });

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItem(tr("All"), static_cast<int>(NoteRepository::NoteListFilter::All));
    m_filterCombo->addItem(tr("Active"), static_cast<int>(NoteRepository::NoteListFilter::Active));
    m_filterCombo->addItem(tr("Archived"), static_cast<int>(NoteRepository::NoteListFilter::Archived));
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
    auto *importButton = new QPushButton(tr("Import"), this);
    connect(importButton, &QPushButton::clicked, this, [this] {
        importNotes();
    });
    auto *backupButton = new QPushButton(tr("Backup"), this);
    connect(backupButton, &QPushButton::clicked, this, [this] {
        exportBackup();
    });
    auto *markdownButton = new QPushButton(tr("Markdown"), this);
    connect(markdownButton, &QPushButton::clicked, this, [this] {
        exportMarkdown();
    });
    auto *textButton = new QPushButton(tr("Text"), this);
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
    m_titleEdit->setPlaceholderText(tr("Title"));
    connect(m_titleEdit, &QLineEdit::textChanged, this, [this] {
        scheduleSave();
    });
    editorLayout->addWidget(m_titleEdit);

    m_tagsEdit = new QLineEdit(this);
    m_tagsEdit->setPlaceholderText(tr("Tags"));
    connect(m_tagsEdit, &QLineEdit::textChanged, this, [this] {
        scheduleSave();
    });
    editorLayout->addWidget(m_tagsEdit);

    m_bodyEdit = new MarkdownEditor(this);
    m_bodyEdit->setObjectName(QStringLiteral("noteBodyEditor"));
    m_bodyEdit->setPlaceholderText(tr("Write a note..."));
    connect(m_bodyEdit, &MarkdownEditor::textChanged, this, [this] {
        scheduleSave();
    });
    editorLayout->addWidget(m_bodyEdit, 1);

    auto *actionsLayout = new QHBoxLayout;
    m_historyUndoButton = new QPushButton(this);
    m_historyUndoButton->setFixedSize(28, 26);
    AppIcons::setButtonIcon(m_historyUndoButton, AppIcons::Icon::Undo, tr("Restore the previous saved version"));
    connect(m_historyUndoButton, &QPushButton::clicked, this, &AllNotesWindow::undoCurrentNote);

    m_historyRedoButton = new QPushButton(this);
    m_historyRedoButton->setFixedSize(28, 26);
    AppIcons::setButtonIcon(m_historyRedoButton, AppIcons::Icon::Redo, tr("Restore the next saved version"));
    connect(m_historyRedoButton, &QPushButton::clicked, this, &AllNotesWindow::redoCurrentNote);

    m_archiveRestoreButton = new QPushButton(this);
    m_archiveRestoreButton->setFixedSize(28, 26);
    AppIcons::setButtonIcon(m_archiveRestoreButton, AppIcons::Icon::Archive, tr("Archive note"));
    connect(m_archiveRestoreButton, &QPushButton::clicked, this, [this] {
        archiveOrRestoreCurrentNote();
    });

    m_deleteButton = new QPushButton(this);
    m_deleteButton->setFixedSize(28, 26);
    AppIcons::setButtonIcon(m_deleteButton, AppIcons::Icon::Delete, tr("Delete note"));
    connect(m_deleteButton, &QPushButton::clicked, this, [this] {
        deleteCurrentNote();
    });

    m_undoButton = new QPushButton(this);
    m_undoButton->setFixedSize(28, 26);
    AppIcons::setButtonIcon(m_undoButton, AppIcons::Icon::Undo, tr("Undo delete"));
    m_undoButton->hide();
    connect(m_undoButton, &QPushButton::clicked, this, [this] {
        undoDelete();
    });

    m_statusLabel = new QLabel(this);
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
    updateStatus(tr("Editing..."));
}

void AllNotesWindow::saveCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    Note &note = m_notes[m_currentNoteIndex];
    note.title = m_titleEdit->text().trimmed();
    if (note.title.isEmpty()) {
        note.title = tr("Untitled");
    }
    note.tags = m_tagsEdit->text().trimmed();
    note.body = m_bodyEdit->storage();

    QString errorMessage;
    if (!m_repository->saveNote(note, &errorMessage)) {
        updateStatus(tr("Save error: %1").arg(errorMessage));
        return;
    }

    updateStatus(tr("Saved"));
    updateHistoryButtons();
    updateList();
    notifyNotesChanged();
}

void AllNotesWindow::archiveOrRestoreCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    Note &note = m_notes[m_currentNoteIndex];
    if (!note.archived
        && QMessageBox::question(this,
                                  tr("Archive note"),
                                  tr("Archive \"%1\"?").arg(note.title),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    saveCurrentNote();
    QString errorMessage;
    const bool ok = note.archived
        ? m_repository->restoreNote(note.id, &errorMessage)
        : m_repository->archiveNote(note.id, &errorMessage);

    if (!ok) {
        updateStatus(tr("State error: %1").arg(errorMessage));
        return;
    }

    updateStatus(note.archived ? tr("Restored") : tr("Archived"));
    notifyNotesChanged();
    refresh();
}

void AllNotesWindow::deleteCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        return;
    }

    const Note &note = m_notes.at(m_currentNoteIndex);
    if (QMessageBox::question(this,
                              tr("Delete note"),
                              tr("Delete \"%1\"?").arg(note.title),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    finalizePendingDelete();
    saveCurrentNote();

    const int deletedIndex = m_currentNoteIndex;
    m_pendingDeleteNoteId = m_notes.at(deletedIndex).id;

    QString errorMessage;
    if (!m_repository->markNoteDeleted(m_pendingDeleteNoteId, &errorMessage)) {
        updateStatus(tr("Delete error: %1").arg(errorMessage));
        m_pendingDeleteNoteId = 0;
        return;
    }

    m_notes.removeAt(deletedIndex);
    updateList();
    selectNoteByIndex(std::min(deletedIndex, static_cast<int>(m_notes.size()) - 1));
    m_undoButton->show();
    m_deleteUndoTimer->start();
    updateStatus(tr("Deleted"));
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
        updateStatus(tr("Undo error: %1").arg(errorMessage));
        return;
    }

    updateStatus(tr("Restored"));
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
        updateStatus(tr("Undo error: %1").arg(errorMessage));
        return;
    }
    m_notes[m_currentNoteIndex] = restoredNote;
    updateEditor();
    updateList();
    updateStatus(tr("Previous version restored"));
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
        updateStatus(tr("Redo error: %1").arg(errorMessage));
        return;
    }
    m_notes[m_currentNoteIndex] = restoredNote;
    updateEditor();
    updateList();
    updateStatus(tr("Next version restored"));
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
        updateStatus(tr("Delete cleanup error: %1").arg(errorMessage));
    }
}

void AllNotesWindow::importNotes()
{
    const QStringList filePaths = QFileDialog::getOpenFileNames(this,
                                                                tr("Import Notes"),
                                                                QString(),
                                                                tr("Notes (*.fnotes *.md *.txt)"));
    if (filePaths.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::importFiles(filePaths, m_repository, &errorMessage)) {
        updateStatus(tr("Import error: %1").arg(errorMessage));
        return;
    }

    updateStatus(tr("Imported"));
    notifyNotesChanged();
    refresh();
}

void AllNotesWindow::exportBackup()
{
    const QString filePath = QFileDialog::getSaveFileName(this, tr("Export Backup"), QStringLiteral("floating-notes.fnotes"), tr("Floating Notes (*.fnotes)"));
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::exportBackup(filePath, exportNotes(), &errorMessage)) {
        updateStatus(tr("Export error: %1").arg(errorMessage));
        return;
    }
    updateStatus(tr("Backup exported"));
}

void AllNotesWindow::exportMarkdown()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Export Markdown"));
    if (directoryPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::exportMarkdown(directoryPath, exportNotes(), &errorMessage)) {
        updateStatus(tr("Export error: %1").arg(errorMessage));
        return;
    }
    updateStatus(tr("Markdown exported"));
}

void AllNotesWindow::exportText()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Export Text"));
    if (directoryPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!ImportExport::exportText(directoryPath, exportNotes(), &errorMessage)) {
        updateStatus(tr("Export error: %1").arg(errorMessage));
        return;
    }
    updateStatus(tr("Text exported"));
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
    updateHistoryButtons();
    m_archiveRestoreButton->setEnabled(hasNote);
    m_deleteButton->setEnabled(hasNote);

    if (hasNote) {
        const Note &note = m_notes.at(m_currentNoteIndex);
        m_titleEdit->setText(note.title);
        m_tagsEdit->setText(note.tags);
        m_bodyEdit->setMarkdown(note.body);
        const QColor ink = usesDarkTheme(m_settings) ? QColor(QStringLiteral("#f2eee6")) : editorInkColor(note.color);
        m_bodyEdit->setStyleSheet(richTextEditorStyle(ink));
        AppIcons::setButtonIcon(m_archiveRestoreButton, AppIcons::Icon::Archive, note.archived ? tr("Restore note") : tr("Archive note"));
    } else {
        m_titleEdit->clear();
        m_tagsEdit->clear();
        m_bodyEdit->setMarkdown({});
        const QColor ink = usesDarkTheme(m_settings) ? QColor(QStringLiteral("#f2eee6")) : QColor(QStringLiteral("#38252d"));
        m_bodyEdit->setStyleSheet(richTextEditorStyle(ink));
        AppIcons::setButtonIcon(m_archiveRestoreButton, AppIcons::Icon::Archive, tr("Archive note"));
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
