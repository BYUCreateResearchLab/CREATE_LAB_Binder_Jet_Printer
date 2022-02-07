#include "mainwindow.h"
#include <QApplication>
#include "printer.h"
#include "printhread.h"

int main(int argc, char *argv[])
{
    Printer printer;
    QApplication a(argc, argv);
    PrintThread printerThread;
    printerThread.setup(&printer);

    MainWindow w;
    w.setup(&printer, &printerThread);
    w.setWindowState(Qt::WindowMaximized); // Set window to be maximized
    w.show();

    int ret{a.exec()}; // returns when application closes

    return ret;
}
