#include "WindowsStartupIntegration.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>

namespace {
constexpr wchar_t StartupShortcutName[] = L"Floating Notes.lnk";

QString hresultText(HRESULT result)
{
    return QStringLiteral("0x%1").arg(static_cast<quint32>(result), 8, 16, QLatin1Char('0'));
}

QString normalizedPath(const QString &path)
{
    const QFileInfo fileInfo(path);
    const QString canonicalPath = fileInfo.canonicalFilePath();
    return canonicalPath.isEmpty() ? fileInfo.absoluteFilePath() : canonicalPath;
}

bool removeLegacyRunEntry(QString *errorMessage)
{
    QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     QSettings::NativeFormat);
    runKey.remove(QCoreApplication::applicationName());
    if (runKey.status() == QSettings::NoError) {
        return true;
    }

    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Could not remove the legacy Windows startup registry entry.");
    }
    return false;
}

void removeStartupApproval(const QString &shortcutName)
{
    QSettings startupApproved(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder"),
        QSettings::NativeFormat);
    startupApproved.remove(shortcutName);
}

bool startupShortcutPath(QString *path, QString *errorMessage)
{
    const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninitializeCom = SUCCEEDED(initializeResult);
    if (FAILED(initializeResult) && initializeResult != RPC_E_CHANGED_MODE) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not initialize Windows startup integration (%1).").arg(hresultText(initializeResult));
        }
        return false;
    }

    PWSTR startupDirectory = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_Startup, KF_FLAG_DEFAULT, nullptr, &startupDirectory);
    if (FAILED(result)) {
        if (uninitializeCom) {
            CoUninitialize();
        }
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not locate the Windows Startup folder (%1).").arg(hresultText(result));
        }
        return false;
    }

    const QString directory = QString::fromWCharArray(startupDirectory);
    CoTaskMemFree(startupDirectory);
    if (uninitializeCom) {
        CoUninitialize();
    }
    *path = QDir(directory).filePath(QString::fromWCharArray(StartupShortcutName));
    return true;
}

bool shortcutTargetsApplication(const QString &shortcutPath, const QString &applicationPath)
{
    IShellLinkW *shellLink = nullptr;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    IPersistFile *persistFile = nullptr;
    if (SUCCEEDED(result)) {
        result = shellLink->QueryInterface(IID_PPV_ARGS(&persistFile));
    }
    if (SUCCEEDED(result)) {
        result = persistFile->Load(reinterpret_cast<LPCWSTR>(shortcutPath.utf16()), STGM_READ);
    }

    WCHAR targetPath[MAX_PATH] = {};
    if (SUCCEEDED(result)) {
        result = shellLink->GetPath(targetPath, MAX_PATH, nullptr, SLGP_RAWPATH);
    }

    if (persistFile != nullptr) {
        persistFile->Release();
    }
    if (shellLink != nullptr) {
        shellLink->Release();
    }

    return SUCCEEDED(result)
        && normalizedPath(QString::fromWCharArray(targetPath)).compare(normalizedPath(applicationPath), Qt::CaseInsensitive) == 0;
}

bool removeStartupShortcutsForCurrentApplication(const QString &startupDirectory, QString *errorMessage)
{
    const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninitializeCom = SUCCEEDED(initializeResult);
    if (FAILED(initializeResult) && initializeResult != RPC_E_CHANGED_MODE) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not initialize Windows startup integration (%1).").arg(hresultText(initializeResult));
        }
        return false;
    }

    const QString applicationPath = QCoreApplication::applicationFilePath();
    const QDir startupFolder(startupDirectory);
    const QStringList shortcuts = startupFolder.entryList({QStringLiteral("*.lnk")}, QDir::Files | QDir::NoSymLinks);
    for (const QString &shortcutName : shortcuts) {
        const QString shortcutPath = startupFolder.filePath(shortcutName);
        if (shortcutTargetsApplication(shortcutPath, applicationPath) && !QFile::remove(shortcutPath)) {
            if (uninitializeCom) {
                CoUninitialize();
            }
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Could not remove the Windows startup shortcut.");
            }
            return false;
        }
        if (!QFile::exists(shortcutPath)) {
            removeStartupApproval(shortcutName);
        }
    }

    if (uninitializeCom) {
        CoUninitialize();
    }
    return true;
}

bool createStartupShortcut(const QString &shortcutPath, QString *errorMessage)
{
    const QString applicationPath = QCoreApplication::applicationFilePath();
    const QString applicationDirectory = QFileInfo(applicationPath).absolutePath();
    if (!QFileInfo::exists(applicationPath)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("The application executable could not be found.");
        }
        return false;
    }

    const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninitializeCom = SUCCEEDED(initializeResult);
    if (FAILED(initializeResult) && initializeResult != RPC_E_CHANGED_MODE) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Could not initialize Windows startup integration (%1).").arg(hresultText(initializeResult));
        }
        return false;
    }

    IShellLinkW *shellLink = nullptr;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (SUCCEEDED(result)) {
        result = shellLink->SetPath(reinterpret_cast<LPCWSTR>(applicationPath.utf16()));
    }
    if (SUCCEEDED(result)) {
        result = shellLink->SetWorkingDirectory(reinterpret_cast<LPCWSTR>(applicationDirectory.utf16()));
    }
    if (SUCCEEDED(result)) {
        result = shellLink->SetDescription(L"Launch Floating Notes");
    }

    IPersistFile *persistFile = nullptr;
    if (SUCCEEDED(result)) {
        result = shellLink->QueryInterface(IID_PPV_ARGS(&persistFile));
    }
    if (SUCCEEDED(result)) {
        result = persistFile->Save(reinterpret_cast<LPCWSTR>(shortcutPath.utf16()), TRUE);
    }

    if (persistFile != nullptr) {
        persistFile->Release();
    }
    if (shellLink != nullptr) {
        shellLink->Release();
    }
    if (uninitializeCom) {
        CoUninitialize();
    }

    if (FAILED(result) && errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Could not create the Windows startup shortcut (%1).").arg(hresultText(result));
    }
    return SUCCEEDED(result);
}
}

bool WindowsStartupIntegration::setLaunchAtStartup(bool enabled, QString *errorMessage)
{
    QString shortcutPath;
    if (!startupShortcutPath(&shortcutPath, errorMessage)) {
        return false;
    }

    if (enabled) {
        if (!removeStartupShortcutsForCurrentApplication(QFileInfo(shortcutPath).absolutePath(), errorMessage)) {
            return false;
        }
        if (!createStartupShortcut(shortcutPath, errorMessage)) {
            return false;
        }
    } else {
        if (!removeStartupShortcutsForCurrentApplication(QFileInfo(shortcutPath).absolutePath(), errorMessage)) {
            return false;
        }
        if (QFile::exists(shortcutPath) && !QFile::remove(shortcutPath)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Could not remove the Windows startup shortcut.");
            }
            return false;
        }
        removeStartupApproval(QFileInfo(shortcutPath).fileName());
    }

    return removeLegacyRunEntry(errorMessage);
}
