#include "WindowsGlobalHotkeys.h"

#include <QCoreApplication>
#include <QString>

#include <windows.h>

#include <utility>

namespace {
constexpr int NewNoteHotkeyId = 0x4e01;
constexpr int AllNotesHotkeyId = 0x4e02;
constexpr int ArchiveHotkeyId = 0x4e03;

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}
}

WindowsGlobalHotkeys::WindowsGlobalHotkeys(GlobalHotkeys::Callback callback)
    : m_callback(std::move(callback))
{
    QCoreApplication::instance()->installNativeEventFilter(this);
}

WindowsGlobalHotkeys::~WindowsGlobalHotkeys()
{
    if (m_registered) {
        UnregisterHotKey(nullptr, NewNoteHotkeyId);
        UnregisterHotKey(nullptr, AllNotesHotkeyId);
        UnregisterHotKey(nullptr, ArchiveHotkeyId);
    }
    QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool WindowsGlobalHotkeys::registerHotkeys(QString *errorMessage)
{
    const UINT modifiers = MOD_CONTROL | MOD_ALT | MOD_NOREPEAT;
    const bool ok = RegisterHotKey(nullptr, NewNoteHotkeyId, modifiers, 'N')
        && RegisterHotKey(nullptr, AllNotesHotkeyId, modifiers, 'A')
        && RegisterHotKey(nullptr, ArchiveHotkeyId, modifiers, 'L');

    if (!ok) {
        setError(errorMessage, QStringLiteral("Could not register Ctrl+Alt+N/A/L global hotkeys."));
        return false;
    }

    m_registered = true;
    return true;
}

bool WindowsGlobalHotkeys::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    auto *msg = static_cast<MSG *>(message);
    if (msg == nullptr || msg->message != WM_HOTKEY || !m_callback) {
        return false;
    }

    switch (static_cast<int>(msg->wParam)) {
    case NewNoteHotkeyId:
        m_callback(GlobalHotkeys::Action::NewNote);
        return true;
    case AllNotesHotkeyId:
        m_callback(GlobalHotkeys::Action::AllNotes);
        return true;
    case ArchiveHotkeyId:
        m_callback(GlobalHotkeys::Action::ArchiveCurrent);
        return true;
    default:
        return false;
    }
}
