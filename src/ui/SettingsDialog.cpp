#include "SettingsDialog.h"

#include "platform/StartupIntegration.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <utility>

SettingsDialog::SettingsDialog(AppSettings *settings,
                               std::function<void()> settingsChangedCallback,
                               QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_settingsChangedCallback(std::move(settingsChangedCallback))
{
    setWindowTitle(tr("Settings"));
    resize(390, 380);
    buildUi();
    loadSettings();
}

void SettingsDialog::buildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(10);

    auto *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem(tr("English"), static_cast<int>(AppSettings::Language::English));
    m_languageCombo->addItem(tr("Spanish"), static_cast<int>(AppSettings::Language::Spanish));
    formLayout->addRow(tr("Language"), m_languageCombo);

    m_edgeCombo = new QComboBox(this);
    m_edgeCombo->addItem(tr("Right"), static_cast<int>(AppSettings::Edge::Right));
    m_edgeCombo->addItem(tr("Left"), static_cast<int>(AppSettings::Edge::Left));
    formLayout->addRow(tr("Edge"), m_edgeCombo);

    m_maxVisibleSpin = new QSpinBox(this);
    m_maxVisibleSpin->setRange(1, 8);
    formLayout->addRow(tr("Visible notes"), m_maxVisibleSpin);

    m_animationSpin = new QSpinBox(this);
    m_animationSpin->setRange(0, 500);
    m_animationSpin->setSuffix(tr(" ms"));
    formLayout->addRow(tr("Animation"), m_animationSpin);

    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem(tr("System"), static_cast<int>(AppSettings::Theme::System));
    m_themeCombo->addItem(tr("Light"), static_cast<int>(AppSettings::Theme::Light));
    m_themeCombo->addItem(tr("Dark"), static_cast<int>(AppSettings::Theme::Dark));
    formLayout->addRow(tr("Theme"), m_themeCombo);

    m_noteFontCombo = new QComboBox(this);
    m_noteFontCombo->addItem(tr("Playful"), static_cast<int>(AppSettings::NoteFont::Playful));
    m_noteFontCombo->addItem(tr("Handwritten"), static_cast<int>(AppSettings::NoteFont::Handwritten));
    m_noteFontCombo->addItem(tr("Rounded"), static_cast<int>(AppSettings::NoteFont::Rounded));
    m_noteFontCombo->addItem(tr("Clean"), static_cast<int>(AppSettings::NoteFont::Clean));
    m_noteFontCombo->addItem(tr("Classic"), static_cast<int>(AppSettings::NoteFont::Classic));
    formLayout->addRow(tr("Note font"), m_noteFontCombo);

    m_noteBodyFontSizeSpin = new QSpinBox(this);
    m_noteBodyFontSizeSpin->setRange(8, 32);
    m_noteBodyFontSizeSpin->setSuffix(tr(" pt"));
    formLayout->addRow(tr("Note body size"), m_noteBodyFontSizeSpin);

    m_rememberNoteWindowSizeCheck = new QCheckBox(tr("Remember note window size"), this);
    formLayout->addRow(QString(), m_rememberNoteWindowSizeCheck);

    m_rememberNoteWindowPositionCheck = new QCheckBox(tr("Remember note window position"), this);
    formLayout->addRow(QString(), m_rememberNoteWindowPositionCheck);

    m_startupCheck = new QCheckBox(tr("Launch when Windows starts"), this);
    formLayout->addRow(QString(), m_startupCheck);

    rootLayout->addLayout(formLayout);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setMinimumHeight(18);
    rootLayout->addWidget(m_statusLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close, this);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, [this] {
        applySettings();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, [this] {
        close();
    });
    rootLayout->addWidget(buttons);
}

void SettingsDialog::loadSettings()
{
    m_languageCombo->setCurrentIndex(m_languageCombo->findData(static_cast<int>(m_settings->language())));
    m_edgeCombo->setCurrentIndex(m_edgeCombo->findData(static_cast<int>(m_settings->preferredEdge())));
    m_maxVisibleSpin->setValue(m_settings->maxVisibleNotes());
    m_animationSpin->setValue(m_settings->animationDurationMs());
    m_themeCombo->setCurrentIndex(m_themeCombo->findData(static_cast<int>(m_settings->theme())));
    m_noteFontCombo->setCurrentIndex(m_noteFontCombo->findData(static_cast<int>(m_settings->noteFont())));
    m_noteBodyFontSizeSpin->setValue(m_settings->noteBodyFontSize());
    m_rememberNoteWindowSizeCheck->setChecked(m_settings->rememberNoteWindowSize());
    m_rememberNoteWindowPositionCheck->setChecked(m_settings->rememberNoteWindowPosition());
    m_startupCheck->setChecked(m_settings->launchAtStartup());
}

void SettingsDialog::applySettings()
{
    const AppSettings::Language previousLanguage = m_settings->language();
    const AppSettings::Language selectedLanguage = static_cast<AppSettings::Language>(m_languageCombo->currentData().toInt());
    m_settings->setLanguage(selectedLanguage);
    m_settings->setPreferredEdge(static_cast<AppSettings::Edge>(m_edgeCombo->currentData().toInt()));
    m_settings->setMaxVisibleNotes(m_maxVisibleSpin->value());
    m_settings->setAnimationDurationMs(m_animationSpin->value());
    m_settings->setTheme(static_cast<AppSettings::Theme>(m_themeCombo->currentData().toInt()));
    m_settings->setNoteFont(static_cast<AppSettings::NoteFont>(m_noteFontCombo->currentData().toInt()));
    m_settings->setNoteBodyFontSize(m_noteBodyFontSizeSpin->value());
    m_settings->setRememberNoteWindowSize(m_rememberNoteWindowSizeCheck->isChecked());
    m_settings->setRememberNoteWindowPosition(m_rememberNoteWindowPositionCheck->isChecked());

    const bool startupEnabled = m_startupCheck->isChecked();
    QString errorMessage;
    if (StartupIntegration::setLaunchAtStartup(startupEnabled, &errorMessage)) {
        m_settings->setLaunchAtStartup(startupEnabled);
    } else if (startupEnabled) {
        setStatus(tr("Startup error: %1").arg(errorMessage));
        return;
    } else {
        m_settings->setLaunchAtStartup(false);
    }

    if (m_settingsChangedCallback) {
        m_settingsChangedCallback();
    }
    setStatus(previousLanguage == selectedLanguage
        ? tr("Saved")
        : tr("Saved. Language changes take effect after restarting the application."));
}

void SettingsDialog::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}
