#include "app/AppController.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setApplicationName("Floating Notes");
    QApplication::setApplicationDisplayName("Floating Notes");
    QApplication::setOrganizationName("Floating Notes");

    AppController controller;
    QString errorMessage;
    if (!controller.start(&errorMessage)) {
        QMessageBox::critical(nullptr, "Floating Notes", errorMessage);
        return 1;
    }

    return QApplication::exec();
}
