#include <QApplication>
#include <QTranslator>
#include <QDebug>

#include "mainwindow.h"
#include "machineGrbl.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;
    if (!translator.load("app_fr"))
        qDebug() << "Can't load translation";
    if (!app.installTranslator(&translator))
        qDebug() << "Can't install translation";

    MainWindow win;
    win.show();

    return app.exec();
}
