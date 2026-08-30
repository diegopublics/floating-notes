#include "AppController.h"

#include "ui/AllNotesWindow.h"
#include "ui/AppTheme.h"
#include "ui/EdgeDockWindow.h"
#include "ui/SettingsDialog.h"

#include <QAction>
#include <QApplication>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QSystemTrayIcon>

#include <algorithm>

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

AppController::~AppController()
{
    qDeleteAll(m_docks);
    delete m_allNotesWindow;
    delete m_settingsDialog;
    delete m_trayMenu;
}

bool AppController::start(QString *errorMessage)
{
    if (!m_repository.open(errorMessage)) {
        return false;
    }

    applySettings();
    rebuildDocks();
    buildTray();

    m_hotkeys = std::make_unique<GlobalHotkeys>([this](GlobalHotkeys::Action action) {
        handleHotkey(action);
    });
    QString hotkeyError;
    m_hotkeys->registerHotkeys(&hotkeyError);

    connect(qApp, &QGuiApplication::screenAdded, this, [this] {
        rebuildDocks();
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this] {
        rebuildDocks();
    });
    return true;
}

void AppController::buildTray()
{
    if (m_trayIcon != nullptr) {
        return;
    }

    m_trayMenu = new QMenu;
    m_trayMenu->addAction("New Note", this, [this] {
        createNote();
    });
    m_trayMenu->addAction("All Notes", this, [this] {
        showAllNotes();
    });
    m_trayMenu->addAction("Archive Current", this, [this] {
        archiveCurrentNote();
    });
    m_trayMenu->addAction("Settings", this, [this] {
        showSettings();
    });
    m_trayMenu->addSeparator();
    m_trayMenu->addAction("Show Docks", this, [this] {
        showDocks();
    });
    m_trayMenu->addAction("Hide Docks", this, [this] {
        hideDocks();
    });
    m_trayMenu->addAction("Exit", this, [this] {
        exitApplication();
    });

    m_trayIcon = new QSystemTrayIcon(trayIcon(), this);
    m_trayIcon->setToolTip("Floating Notes");
    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showAllNotes();
        }
    });
    m_trayIcon->show();
}

void AppController::rebuildDocks()
{
    for (EdgeDockWindow *dock : m_docks) {
        dock->saveCurrentNoteNow();
    }

    qDeleteAll(m_docks);
    m_docks.clear();

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen == nullptr) {
        return;
    }

    auto *dock = new EdgeDockWindow(&m_repository,
                                    &m_settings,
                                    screen,
                                    [this] { showAllNotes(); },
                                    [this] { showSettings(); },
                                    [this] { hideDocks(); });
    m_docks.append(dock);
    connect(screen, &QScreen::geometryChanged, dock, [dock] {
        dock->applySettings();
    });
    dock->show();
}

void AppController::applySettings()
{
    qApp->setStyleSheet(AppTheme::applicationStyleSheet(m_settings.theme()));
    for (EdgeDockWindow *dock : m_docks) {
        dock->applySettings();
    }
}

void AppController::createNote()
{
    if (m_docks.isEmpty()) {
        rebuildDocks();
    }
    if (!m_docks.isEmpty()) {
        showDocks();
        m_docks.first()->createNoteAndFocus();
    }
}

void AppController::archiveCurrentNote()
{
    if (!m_docks.isEmpty()) {
        m_docks.first()->archiveSelectedNote();
    }
}

void AppController::showAllNotes()
{
    if (m_allNotesWindow == nullptr) {
        m_allNotesWindow = new AllNotesWindow(&m_repository, [this] {
            refreshAllDocks();
        });
    }

    m_allNotesWindow->refresh();
    m_allNotesWindow->show();
    m_allNotesWindow->raise();
    m_allNotesWindow->activateWindow();
}

void AppController::showSettings()
{
    if (m_settingsDialog == nullptr) {
        m_settingsDialog = new SettingsDialog(&m_settings, [this] {
            applySettings();
            rebuildDocks();
            refreshAllDocks();
        });
    }

    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void AppController::hideDocks()
{
    for (EdgeDockWindow *dock : m_docks) {
        dock->hide();
    }
}

void AppController::showDocks()
{
    for (EdgeDockWindow *dock : m_docks) {
        dock->show();
    }
}

void AppController::exitApplication()
{
    for (EdgeDockWindow *dock : m_docks) {
        dock->saveCurrentNoteNow();
    }
    if (m_allNotesWindow != nullptr) {
        m_allNotesWindow->saveCurrentNoteNow();
    }
    QApplication::quit();
}

void AppController::refreshAllDocks()
{
    for (EdgeDockWindow *dock : m_docks) {
        dock->refreshNotes();
    }
}

void AppController::handleHotkey(GlobalHotkeys::Action action)
{
    switch (action) {
    case GlobalHotkeys::Action::NewNote:
        createNote();
        break;
    case GlobalHotkeys::Action::AllNotes:
        showAllNotes();
        break;
    case GlobalHotkeys::Action::ArchiveCurrent:
        archiveCurrentNote();
        break;
    }
}

QIcon AppController::trayIcon() const
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#26231d"), 2));
    painter.setBrush(QColor("#e8b457"));
    painter.drawRoundedRect(QRectF(5, 4, 22, 24), 5, 5);
    painter.drawLine(QPointF(10, 12), QPointF(22, 12));
    painter.drawLine(QPointF(10, 18), QPointF(20, 18));
    return QIcon(pixmap);
}
