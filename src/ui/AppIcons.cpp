#include "AppIcons.h"

#include <QPushButton>
#include <QSize>

namespace AppIcons {

QIcon icon(Icon icon)
{
    switch (icon) {
    case Icon::Undo: return QIcon(QStringLiteral(":/icons/fontawesome/arrow-rotate-left.svg"));
    case Icon::Redo: return QIcon(QStringLiteral(":/icons/fontawesome/arrow-rotate-right.svg"));
    case Icon::Pin: return QIcon(QStringLiteral(":/icons/fontawesome/thumbtack.svg"));
    case Icon::Checklist: return QIcon(QStringLiteral(":/icons/fontawesome/list-check.svg"));
    case Icon::AllNotes: return QIcon(QStringLiteral(":/icons/fontawesome/list.svg"));
    case Icon::Archive: return QIcon(QStringLiteral(":/icons/fontawesome/box-archive.svg"));
    case Icon::Delete: return QIcon(QStringLiteral(":/icons/fontawesome/trash.svg"));
    case Icon::Close: return QIcon(QStringLiteral(":/icons/fontawesome/xmark.svg"));
    case Icon::Create: return QIcon(QStringLiteral(":/icons/fontawesome/plus.svg"));
    }
    return {};
}

void setButtonIcon(QPushButton *button, Icon iconValue, const QString &description)
{
    button->setText({});
    button->setIcon(icon(iconValue));
    button->setIconSize(QSize(15, 15));
    button->setToolTip(description);
    button->setAccessibleName(description);
}

}
