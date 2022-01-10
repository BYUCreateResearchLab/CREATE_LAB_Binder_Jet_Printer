#include "mainwindow.h"
#include <QApplication>
#include "printer.h"

int main(int argc, char *argv[])
{
    Printer *printer{new Printer};
    QApplication a(argc, argv);
    MainWindow w;
    w.setup(printer);
    w.setWindowState(Qt::WindowMaximized); // Set window to be maximized
    w.show();

    int ret{a.exec()}; // returns when application closes
    delete printer;
    return ret;
}
