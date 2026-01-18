#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("ControlTune Pro");  // Aquí se define el nombre en la interfaz
    w.show();
    return a.exec();
}
