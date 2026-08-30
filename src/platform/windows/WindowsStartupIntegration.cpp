#include "WindowsStartupIntegration.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

bool WindowsStartupIntegration::setLaunchAtStartup(bool enabled, QString *errorMessage)
{
    QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     QSettings::NativeFormat);
    const QString appName = QCoreApplication::applicationName();
    if (enabled) {
        runKey.setValue(appName, QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    } else {
        runKey.remove(appName);
    }

    if (runKey.status() != QSettings::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not update the Windows startup registry key.");
        }
        return false;
    }
    return true;
}
