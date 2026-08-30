#pragma once

#include "app/AppSettings.h"

#include <QString>

class AppTheme
{
public:
    static QString applicationStyleSheet(AppSettings::Theme theme);
};
