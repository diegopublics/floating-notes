#pragma once

#include "app/AppSettings.h"

#include <QDialog>

#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;

class SettingsDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(AppSettings *settings,
                            std::function<void()> settingsChangedCallback,
                            QWidget *parent = nullptr);

private:
    void buildUi();
    void loadSettings();
    void applySettings();
    void setStatus(const QString &text);

    AppSettings *m_settings = nullptr;
    std::function<void()> m_settingsChangedCallback;
    QComboBox *m_languageCombo = nullptr;
    QComboBox *m_edgeCombo = nullptr;
    QSpinBox *m_maxVisibleSpin = nullptr;
    QSpinBox *m_animationSpin = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QComboBox *m_noteFontCombo = nullptr;
    QSpinBox *m_noteBodyFontSizeSpin = nullptr;
    QCheckBox *m_rememberNoteWindowSizeCheck = nullptr;
    QCheckBox *m_rememberNoteWindowPositionCheck = nullptr;
    QCheckBox *m_startupCheck = nullptr;
    QLabel *m_statusLabel = nullptr;
};
