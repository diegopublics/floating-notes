#pragma once

#include <QString>

namespace StartupIntegration {
bool setLaunchAtStartup(bool enabled, QString *errorMessage = nullptr);
}
