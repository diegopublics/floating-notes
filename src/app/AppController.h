#pragma once

#include "AppSettings.h"
#include "persistence/NoteRepository.h"
#include "platform/GlobalHotkeys.h"

#include <QObject>
#include <QVector>

#include <memory>

class QAction;
class AllNotesWindow;
class EdgeDockWindow;
class QMenu;
class QScreen;
class QSystemTrayIcon;
class SettingsDialog;

class AppController final : public QObject
{
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    bool start(QString *errorMessage = nullptr);

private:
    void buildTray();
    void rebuildDocks();
    void applySettings();
    void createNote();
    void archiveCurrentNote();
    void showAllNotes();
    void showSettings();
    void hideDocks();
    void showDocks();
    void exitApplication();
    void refreshAllDocks();
    void handleHotkey(GlobalHotkeys::Action action);
    QIcon trayIcon() const;

    NoteRepository m_repository;
    AppSettings m_settings;
    QVector<EdgeDockWindow *> m_docks;
    AllNotesWindow *m_allNotesWindow = nullptr;
    SettingsDialog *m_settingsDialog = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    std::unique_ptr<GlobalHotkeys> m_hotkeys;
};
