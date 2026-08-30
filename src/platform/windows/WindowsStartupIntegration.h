#pragma once

#include <QString>

namespace WindowsStartupIntegration {
bool setLaunchAtStartup(bool enabled, QString *errorMessage = nullptr);
}
