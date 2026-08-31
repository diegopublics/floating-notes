#include "HelpDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Keyboard Shortcuts"));
    setMinimumWidth(340);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *description = new QLabel(QStringLiteral("Global shortcuts"), this);
    QFont titleFont = description->font();
    titleFont.setWeight(QFont::DemiBold);
    description->setFont(titleFont);
    layout->addWidget(description);

    auto *shortcuts = new QFormLayout;
    shortcuts->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    shortcuts->addRow(QStringLiteral("Ctrl+Alt+N"), new QLabel(QStringLiteral("Create a new note"), this));
    shortcuts->addRow(QStringLiteral("Ctrl+Alt+A"), new QLabel(QStringLiteral("Show all notes"), this));
    shortcuts->addRow(QStringLiteral("Ctrl+Alt+L"), new QLabel(QStringLiteral("Archive the current note"), this));
    layout->addLayout(shortcuts);

    auto *hint = new QLabel(QStringLiteral("Esc closes an open note editor."), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);
}
