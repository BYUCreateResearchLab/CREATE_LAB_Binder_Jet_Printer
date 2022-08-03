#include "mainwindow.h"
#include <QApplication>
#include "printer.h"
#include "printhread.h"

int main(int argc, char *argv[])
{
    // initialize objects that will persist through the life of the program
    Printer printer;
    QApplication a(argc, argv);
    PrintThread printerThread;
    printerThread.setup(&printer);

    // Initialize the main window
    MainWindow w;
    w.setup(&printer, &printerThread);
    w.setWindowState(Qt::WindowMaximized); // Set window to be maximized
    w.show();

    int ret{a.exec()}; // returns when application closes

    return ret;
}
