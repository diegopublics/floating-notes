#include "StartupIntegration.h"

#if defined(Q_OS_WIN)
#include "windows/WindowsStartupIntegration.h"
#endif

bool StartupIntegration::setLaunchAtStartup(bool enabled, QString *errorMessage)
{
#if defined(Q_OS_WIN)
    return WindowsStartupIntegration::setLaunchAtStartup(enabled, errorMessage);
#else
    Q_UNUSED(enabled)
    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Startup integration is not implemented on this platform yet.");
    }
    return false;
#endif
}
