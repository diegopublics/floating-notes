#pragma once

#include "core/Note.h"
#include "persistence/NoteRepository.h"

#include <QHash>
#include <QPointer>
#include <QWidget>
#include <QVector>

#include <functional>

class AppSettings;
class QEnterEvent;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QMouseEvent;
class QMoveEvent;
class QPlainTextEdit;
class QPropertyAnimation;
class QPushButton;
class QScreen;
class QTimer;

class EdgeDockWindow final : public QWidget
{
public:
    explicit EdgeDockWindow(NoteRepository *repository,
                            AppSettings *settings,
                            QScreen *screen,
                            std::function<void()> allNotesCallback,
                            std::function<void()> settingsCallback,
                            std::function<void()> hideCallback,
                            QWidget *parent = nullptr);

    void applySettings();
    void refreshNotes();
    void createNoteAndFocus();
    void archiveSelectedNote();
    void openNote(int noteId);
    void saveCurrentNoteNow();
    bool isEditingNote(int noteId) const;
    void moveNoteWindowTo(const QPoint &position);
    void resetNoteWindowPosition();
    void setEditorOnly(bool editorOnly);
    void setOpenNoteCallback(std::function<void(int)> callback);
    void setNotesChangedCallback(std::function<void()> callback);

protected:
    void changeEvent(QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void buildContent();
    void positionNearScreenEdge();
    QRect targetGeometryForWidth(int targetWidth) const;
    void deleteCurrentNote();
    void undoDelete();
    void finalizePendingDelete();
    void insertChecklistLine();
    void selectNote(int index);
    void scheduleSave();
    void saveCurrentNote();
    void scheduleCollapse();
    void setExpanded(bool expanded);
    void setEditorVisible(bool visible);
    void setStatusText(const QString &text);
    int deckHeight() const;
    int expandedWidth() const;
    int visibleNoteCount() const;
    void updateColorButtons();
    void updateContentVisibility();
    void updateEditorFromCurrentNote();
    void updateNoteButtons();
    void updateNoteColor(const QString &color);
    void layoutDeck();
    void layoutEditor();
    int fanHeight() const;
    int fanTabVisibleStrip(int index) const;
    int pillHeight() const;
    QSize noteWindowSize() const;
    QPoint defaultNoteWindowPosition(const QSize &size) const;
    void prepareNoteWindowGeometry();
    bool isNoteWindowDragHandle(const QPoint &position) const;
    bool isNoteWindowResizeHandle(const QPoint &position) const;
    void rememberNoteWindowGeometry();
    int currentNoteId() const;

    NoteRepository *m_repository = nullptr;
    AppSettings *m_settings = nullptr;
    QPointer<QScreen> m_screen;
    std::function<void()> m_allNotesCallback;
    std::function<void()> m_settingsCallback;
    std::function<void()> m_hideCallback;
    std::function<void(int)> m_openNoteCallback;
    std::function<void()> m_notesChangedCallback;
    QVector<Note> m_notes;
    QVector<QPushButton *> m_noteButtons;
    QVector<QPushButton *> m_colorButtons;
    QPushButton *m_createButton = nullptr;
    QPushButton *m_moreButton = nullptr;
    QPushButton *m_allNotesButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPushButton *m_archiveButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_undoButton = nullptr;
    QPushButton *m_checklistButton = nullptr;
    QPushButton *m_pinButton = nullptr;
    QWidget *m_editorPanel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_tagsEdit = nullptr;
    QPlainTextEdit *m_bodyEdit = nullptr;
    QPushButton *m_hideButton = nullptr;
    QPropertyAnimation *m_geometryAnimation = nullptr;
    QTimer *m_collapseTimer = nullptr;
    QTimer *m_deleteUndoTimer = nullptr;
    QTimer *m_saveTimer = nullptr;
    int m_currentNoteIndex = -1;
    int m_pendingDeleteNoteId = 0;
    bool m_expanded = false;
    bool m_editorVisible = false;
    bool m_keepOpen = false;
    bool m_loadingEditor = false;
    bool m_editorOnly = false;
    bool m_draggingNoteWindow = false;
    bool m_resizingNoteWindow = false;
    bool m_hasNoteWindowPosition = false;
    QPoint m_noteWindowPosition;
    QPoint m_noteWindowDragOffset;
    QPoint m_noteWindowResizeStart;
    QPoint m_noteWindowResizeStartPosition;
    QSize m_noteWindowContentSize;
    QSize m_noteWindowResizeStartSize;
    QHash<int, int> m_bodyCursorPositions;
};
