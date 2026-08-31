#pragma once

#include <QPoint>
#include <QSize>
#include <QString>
#include <QVector>

class AppSettings
{
public:
    enum class Edge {
        Left,
        Right,
    };

    enum class Theme {
        System,
        Light,
        Dark,
    };

    enum class NoteFont {
        Playful,
        Handwritten,
        Rounded,
        Clean,
        Classic,
    };

    enum class Language {
        English,
        Spanish,
    };

    Language language() const;
    void setLanguage(Language language);

    Edge preferredEdge() const;
    void setPreferredEdge(Edge edge);

    bool alwaysOnTop() const;
    void setAlwaysOnTop(bool enabled);

    int maxVisibleNotes() const;
    void setMaxVisibleNotes(int count);

    int animationDurationMs() const;
    void setAnimationDurationMs(int durationMs);

    Theme theme() const;
    void setTheme(Theme theme);

    NoteFont noteFont() const;
    void setNoteFont(NoteFont noteFont);

    int noteBodyFontSize() const;
    void setNoteBodyFontSize(int pointSize);

    bool rememberNoteWindowSize() const;
    void setRememberNoteWindowSize(bool enabled);
    QSize noteWindowSize(int noteId) const;
    void setNoteWindowSize(int noteId, const QSize &size);

    bool rememberNoteWindowPosition() const;
    void setRememberNoteWindowPosition(bool enabled);
    bool hasNoteWindowPosition(int noteId) const;
    QPoint noteWindowPosition(int noteId) const;
    void setNoteWindowPosition(int noteId, const QPoint &position);
    void clearNoteWindowPositions();

    int noteBodyCursorPosition(int noteId) const;
    void setNoteBodyCursorPosition(int noteId, int position);

    QVector<int> pinnedNoteIds() const;
    void setPinnedNoteIds(const QVector<int> &noteIds);

    bool restorePinnedNotesOnStartup() const;
    void setRestorePinnedNotesOnStartup(bool enabled);

    bool launchAtStartup() const;
    void setLaunchAtStartup(bool enabled);
};
