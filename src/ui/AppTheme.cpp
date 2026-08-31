#include "AppTheme.h"

#include <QGuiApplication>
#include <QStyleHints>

QString AppTheme::applicationStyleSheet(AppSettings::Theme theme)
{
    if (theme == AppSettings::Theme::System) {
        theme = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark
            ? AppSettings::Theme::Dark
            : AppSettings::Theme::Light;
    }

    if (theme == AppSettings::Theme::Dark) {
        return QStringLiteral(
            "QWidget { background: #24231f; color: #f2eee6; }"
            "QLineEdit, QPlainTextEdit, QTextEdit, QListWidget, QComboBox {"
            "  background: #302e29; color: #f2eee6; border: 1px solid #5a544a; border-radius: 6px; padding: 6px;"
            "}"
            "QPushButton { background: #343129; color: #f2eee6; border: 1px solid #5a544a; border-radius: 6px; padding: 6px 10px; }"
            "QPushButton:hover { background: #403b32; }");
    }

    return QStringLiteral(
        "QWidget { background: #f7f3ea; color: #26231d; }"
            "QLineEdit, QPlainTextEdit, QTextEdit, QListWidget, QComboBox {"
        "  background: #fffdf8; color: #26231d; border: 1px solid #d9d1c2; border-radius: 6px; padding: 6px;"
        "}"
        "QPushButton { background: #fffdf8; color: #26231d; border: 1px solid #d9d1c2; border-radius: 6px; padding: 6px 10px; }"
        "QPushButton:hover { background: #f0eadf; }");
}
