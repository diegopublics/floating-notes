#include "AppSettings.h"

#include <QSettings>

#include <algorithm>

namespace {
constexpr int DefaultMaxVisibleNotes = 8;
constexpr int DefaultAnimationDurationMs = 180;
constexpr int DefaultNoteBodyFontSize = 14;

QString noteWindowSizeKey(int noteId)
{
    return QStringLiteral("noteWindowSizes/%1").arg(noteId);
}

QString noteWindowPositionKey(int noteId)
{
    return QStringLiteral("noteWindowPositions/%1").arg(noteId);
}

QString noteBodyCursorPositionKey(int noteId)
{
    return QStringLiteral("noteBodyCursorPositions/%1").arg(noteId);
}

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

QString languageToString(AppSettings::Language language)
{
    return language == AppSettings::Language::Spanish ? QStringLiteral("es") : QStringLiteral("en");
}

AppSettings::Language languageFromString(const QString &value)
{
    return value == QStringLiteral("es") ? AppSettings::Language::Spanish : AppSettings::Language::English;
}
}

AppSettings::Language AppSettings::language() const
{
    return languageFromString(QSettings().value(QStringLiteral("general/language"), QStringLiteral("en")).toString());
}

void AppSettings::setLanguage(Language language)
{
    QSettings().setValue(QStringLiteral("general/language"), languageToString(language));
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

int AppSettings::noteBodyFontSize() const
{
    return qBound(8, QSettings().value(QStringLiteral("appearance/noteBodyFontSize"), DefaultNoteBodyFontSize).toInt(), 32);
}

void AppSettings::setNoteBodyFontSize(int pointSize)
{
    QSettings().setValue(QStringLiteral("appearance/noteBodyFontSize"), qBound(8, pointSize, 32));
}

bool AppSettings::rememberNoteWindowSize() const
{
    return QSettings().value(QStringLiteral("appearance/rememberNoteWindowSize"), true).toBool();
}

void AppSettings::setRememberNoteWindowSize(bool enabled)
{
    QSettings().setValue(QStringLiteral("appearance/rememberNoteWindowSize"), enabled);
}

QSize AppSettings::noteWindowSize(int noteId) const
{
    return QSettings().value(noteWindowSizeKey(noteId)).toSize();
}

void AppSettings::setNoteWindowSize(int noteId, const QSize &size)
{
    QSettings().setValue(noteWindowSizeKey(noteId), size);
}

bool AppSettings::rememberNoteWindowPosition() const
{
    return QSettings().value(QStringLiteral("appearance/rememberNoteWindowPosition"), true).toBool();
}

void AppSettings::setRememberNoteWindowPosition(bool enabled)
{
    QSettings().setValue(QStringLiteral("appearance/rememberNoteWindowPosition"), enabled);
}

bool AppSettings::hasNoteWindowPosition(int noteId) const
{
    return QSettings().contains(noteWindowPositionKey(noteId));
}

QPoint AppSettings::noteWindowPosition(int noteId) const
{
    return QSettings().value(noteWindowPositionKey(noteId)).toPoint();
}

void AppSettings::setNoteWindowPosition(int noteId, const QPoint &position)
{
    QSettings().setValue(noteWindowPositionKey(noteId), position);
}

void AppSettings::clearNoteWindowPositions()
{
    QSettings().remove(QStringLiteral("noteWindowPositions"));
}

int AppSettings::noteBodyCursorPosition(int noteId) const
{
    return std::max(0, QSettings().value(noteBodyCursorPositionKey(noteId), 0).toInt());
}

void AppSettings::setNoteBodyCursorPosition(int noteId, int position)
{
    QSettings().setValue(noteBodyCursorPositionKey(noteId), std::max(0, position));
}

bool AppSettings::launchAtStartup() const
{
    return QSettings().value(QStringLiteral("general/launchAtStartup"), false).toBool();
}

void AppSettings::setLaunchAtStartup(bool enabled)
{
    QSettings().setValue(QStringLiteral("general/launchAtStartup"), enabled);
}
