#pragma once

#include <QString>

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

    Edge preferredEdge() const;
    void setPreferredEdge(Edge edge);

    int maxVisibleNotes() const;
    void setMaxVisibleNotes(int count);

    int animationDurationMs() const;
    void setAnimationDurationMs(int durationMs);

    Theme theme() const;
    void setTheme(Theme theme);

    NoteFont noteFont() const;
    void setNoteFont(NoteFont noteFont);

    bool launchAtStartup() const;
    void setLaunchAtStartup(bool enabled);
};
