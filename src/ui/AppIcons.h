#pragma once

#include <QIcon>

class QPushButton;

namespace AppIcons {

enum class Icon {
    Undo,
    Redo,
    Pin,
    Checklist,
    AllNotes,
    Archive,
    Delete,
    Close,
    Create,
};

QIcon icon(Icon icon);
void setButtonIcon(QPushButton *button, Icon icon, const QString &description);

}
