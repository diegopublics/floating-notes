#include "app/AppController.h"
#include "app/AppSettings.h"

#include <QApplication>
#include <QMessageBox>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setApplicationName("Floating Notes");
    QApplication::setApplicationDisplayName("Floating Notes");
    QApplication::setOrganizationName("Floating Notes");

    AppSettings startupSettings;
    QTranslator translator;
    if (startupSettings.language() == AppSettings::Language::Spanish
        && translator.load(QStringLiteral(":/i18n/FloatingNotes_es.qm"))) {
        app.installTranslator(&translator);
    }

    AppController controller;
    QString errorMessage;
    if (!controller.start(&errorMessage)) {
        QMessageBox::critical(nullptr, QObject::tr("Floating Notes"), errorMessage);
        return 1;
    }

    return QApplication::exec();
}
