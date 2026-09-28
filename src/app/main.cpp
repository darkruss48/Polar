#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QTextEdit>
#include "appsettings.h" // +
#include <QCoreApplication> // FIX: for setApplicationName/Organization

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // FIX: stabiliser l'identité Qt pour les chemins AppData/QSettings,
    // même si l'exécutable est renommé.
    QCoreApplication::setOrganizationName(QStringLiteral("Polar"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("github.com/darkruss48"));
    QCoreApplication::setApplicationName(QStringLiteral("Polar"));
    QTranslator translator;
    if (translator.load(":/in18/en_EN.qm")) {
        a.installTranslator(&translator);
    }

    // Load settings (creates polar.json with defaults if missing)
    AppSettings::load();

    MainWindow w;
    w.show();
    return a.exec();
}
