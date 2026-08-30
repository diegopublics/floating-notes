#include "AppSettings.h"

#include <QSettings>

namespace {
constexpr int DefaultMaxVisibleNotes = 8;
constexpr int DefaultAnimationDurationMs = 180;

QString edgeToString(AppSettings::Edge edge)
{
    return edge == AppSettings::Edge::Left ? QStringLiteral("left") : QStringLiteral("right");
}

AppSettings::Edge edgeFromString(const QString &value)
{
    return value == QStringLiteral("left") ? AppSettings::Edge::Left : AppSettings::Edge::Right;
}

QString themeToString(AppSettings::Theme theme)
{
    switch (theme) {
    case AppSettings::Theme::Light:
        return QStringLiteral("light");
    case AppSettings::Theme::Dark:
        return QStringLiteral("dark");
    case AppSettings::Theme::System:
        return QStringLiteral("system");
    }
    return QStringLiteral("system");
}

AppSettings::Theme themeFromString(const QString &value)
{
    if (value == QStringLiteral("light")) {
        return AppSettings::Theme::Light;
    }
    if (value == QStringLiteral("dark")) {
        return AppSettings::Theme::Dark;
    }
    return AppSettings::Theme::System;
}

QString noteFontToString(AppSettings::NoteFont noteFont)
{
    switch (noteFont) {
    case AppSettings::NoteFont::Playful:
        return QStringLiteral("playful");
    case AppSettings::NoteFont::Handwritten:
        return QStringLiteral("handwritten");
    case AppSettings::NoteFont::Rounded:
        return QStringLiteral("rounded");
    case AppSettings::NoteFont::Clean:
        return QStringLiteral("clean");
    case AppSettings::NoteFont::Classic:
        return QStringLiteral("classic");
    }
    return QStringLiteral("playful");
}

AppSettings::NoteFont noteFontFromString(const QString &value)
{
    if (value == QStringLiteral("handwritten")) {
        return AppSettings::NoteFont::Handwritten;
    }
    if (value == QStringLiteral("rounded")) {
        return AppSettings::NoteFont::Rounded;
    }
    if (value == QStringLiteral("clean")) {
        return AppSettings::NoteFont::Clean;
    }
    if (value == QStringLiteral("classic")) {
        return AppSettings::NoteFont::Classic;
    }
    return AppSettings::NoteFont::Playful;
}
}

AppSettings::Edge AppSettings::preferredEdge() const
{
    return edgeFromString(QSettings().value(QStringLiteral("general/preferredEdge"), QStringLiteral("right")).toString());
}

void AppSettings::setPreferredEdge(Edge edge)
{
    QSettings().setValue(QStringLiteral("general/preferredEdge"), edgeToString(edge));
}

int AppSettings::maxVisibleNotes() const
{
    return QSettings().value(QStringLiteral("general/maxVisibleNotes"), DefaultMaxVisibleNotes).toInt();
}

void AppSettings::setMaxVisibleNotes(int count)
{
    QSettings().setValue(QStringLiteral("general/maxVisibleNotes"), qBound(1, count, 8));
}

int AppSettings::animationDurationMs() const
{
    return QSettings().value(QStringLiteral("general/animationDurationMs"), DefaultAnimationDurationMs).toInt();
}

void AppSettings::setAnimationDurationMs(int durationMs)
{
    QSettings().setValue(QStringLiteral("general/animationDurationMs"), qBound(0, durationMs, 500));
}

AppSettings::Theme AppSettings::theme() const
{
    return themeFromString(QSettings().value(QStringLiteral("appearance/theme"), QStringLiteral("system")).toString());
}

void AppSettings::setTheme(Theme theme)
{
    QSettings().setValue(QStringLiteral("appearance/theme"), themeToString(theme));
}

AppSettings::NoteFont AppSettings::noteFont() const
{
    return noteFontFromString(QSettings().value(QStringLiteral("appearance/noteFont"), QStringLiteral("playful")).toString());
}

void AppSettings::setNoteFont(NoteFont noteFont)
{
    QSettings().setValue(QStringLiteral("appearance/noteFont"), noteFontToString(noteFont));
}

bool AppSettings::launchAtStartup() const
{
    return QSettings().value(QStringLiteral("general/launchAtStartup"), false).toBool();
}

void AppSettings::setLaunchAtStartup(bool enabled)
{
    QSettings().setValue(QStringLiteral("general/launchAtStartup"), enabled);
}
