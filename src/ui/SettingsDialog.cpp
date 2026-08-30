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
    setWindowTitle("Settings");
    resize(360, 240);
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

    m_edgeCombo = new QComboBox(this);
    m_edgeCombo->addItem("Right", static_cast<int>(AppSettings::Edge::Right));
    m_edgeCombo->addItem("Left", static_cast<int>(AppSettings::Edge::Left));
    formLayout->addRow("Edge", m_edgeCombo);

    m_maxVisibleSpin = new QSpinBox(this);
    m_maxVisibleSpin->setRange(1, 8);
    formLayout->addRow("Visible notes", m_maxVisibleSpin);

    m_animationSpin = new QSpinBox(this);
    m_animationSpin->setRange(0, 500);
    m_animationSpin->setSuffix(" ms");
    formLayout->addRow("Animation", m_animationSpin);

    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem("System", static_cast<int>(AppSettings::Theme::System));
    m_themeCombo->addItem("Light", static_cast<int>(AppSettings::Theme::Light));
    m_themeCombo->addItem("Dark", static_cast<int>(AppSettings::Theme::Dark));
    formLayout->addRow("Theme", m_themeCombo);

    m_noteFontCombo = new QComboBox(this);
    m_noteFontCombo->addItem("Playful", static_cast<int>(AppSettings::NoteFont::Playful));
    m_noteFontCombo->addItem("Handwritten", static_cast<int>(AppSettings::NoteFont::Handwritten));
    m_noteFontCombo->addItem("Rounded", static_cast<int>(AppSettings::NoteFont::Rounded));
    m_noteFontCombo->addItem("Clean", static_cast<int>(AppSettings::NoteFont::Clean));
    m_noteFontCombo->addItem("Classic", static_cast<int>(AppSettings::NoteFont::Classic));
    formLayout->addRow("Note font", m_noteFontCombo);

    m_startupCheck = new QCheckBox("Launch when Windows starts", this);
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
    m_edgeCombo->setCurrentIndex(m_edgeCombo->findData(static_cast<int>(m_settings->preferredEdge())));
    m_maxVisibleSpin->setValue(m_settings->maxVisibleNotes());
    m_animationSpin->setValue(m_settings->animationDurationMs());
    m_themeCombo->setCurrentIndex(m_themeCombo->findData(static_cast<int>(m_settings->theme())));
    m_noteFontCombo->setCurrentIndex(m_noteFontCombo->findData(static_cast<int>(m_settings->noteFont())));
    m_startupCheck->setChecked(m_settings->launchAtStartup());
}

void SettingsDialog::applySettings()
{
    m_settings->setPreferredEdge(static_cast<AppSettings::Edge>(m_edgeCombo->currentData().toInt()));
    m_settings->setMaxVisibleNotes(m_maxVisibleSpin->value());
    m_settings->setAnimationDurationMs(m_animationSpin->value());
    m_settings->setTheme(static_cast<AppSettings::Theme>(m_themeCombo->currentData().toInt()));
    m_settings->setNoteFont(static_cast<AppSettings::NoteFont>(m_noteFontCombo->currentData().toInt()));

    const bool startupEnabled = m_startupCheck->isChecked();
    QString errorMessage;
    if (StartupIntegration::setLaunchAtStartup(startupEnabled, &errorMessage)) {
        m_settings->setLaunchAtStartup(startupEnabled);
    } else if (startupEnabled) {
        setStatus(QStringLiteral("Startup error: %1").arg(errorMessage));
        return;
    } else {
        m_settings->setLaunchAtStartup(false);
    }

    if (m_settingsChangedCallback) {
        m_settingsChangedCallback();
    }
    setStatus(QStringLiteral("Saved"));
}

void SettingsDialog::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}
