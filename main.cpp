#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QTextEdit>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator translator;
    if (translator.load(":/in18/en_EN.qm")) {
        a.installTranslator(&translator);
    }


    MainWindow w;
    w.show();
    return a.exec();
}
