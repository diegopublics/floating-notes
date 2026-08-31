#include "HelpDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Keyboard Shortcuts"));
    setMinimumWidth(340);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *description = new QLabel(tr("Global shortcuts"), this);
    QFont titleFont = description->font();
    titleFont.setWeight(QFont::DemiBold);
    description->setFont(titleFont);
    layout->addWidget(description);

    auto *shortcuts = new QFormLayout;
    shortcuts->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    shortcuts->addRow(QStringLiteral("Ctrl+Alt+N"), new QLabel(tr("Create a new note"), this));
    shortcuts->addRow(QStringLiteral("Ctrl+Alt+A"), new QLabel(tr("Show all notes"), this));
    shortcuts->addRow(QStringLiteral("Ctrl+Alt+L"), new QLabel(tr("Archive the current note"), this));
    layout->addLayout(shortcuts);

    auto *hint = new QLabel(tr("Esc closes an open note editor."), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);
}
