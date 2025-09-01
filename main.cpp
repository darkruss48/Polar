#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QTextEdit>
#include "appsettings.h" // +

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
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
