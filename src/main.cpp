#include "mainwindow.h"
#include <QApplication>
#include "printer.h"
#include "printhread.h"

const QPalette dark_palette();

int main(int argc, char *argv[])
{
    // initialize objects that will persist through the life of the program
    Printer printer;
    QApplication a(argc, argv);
    PrintThread printerThread;
    printerThread.setup(&printer);
    a.setStyle("fusion");

    a.setPalette(dark_palette());

    // Initialize the main window
    MainWindow w;
    w.setup(&printer, &printerThread);
    w.setWindowState(Qt::WindowMaximized); // Set window to be maximized
    w.show();

    int ret{a.exec()}; // returns when application closes

    return ret;
}

const QPalette dark_palette()
{
    QColor darkGray(46, 47, 48);
    QColor gray(92, 94, 96);
    QColor black(40, 40, 40);
    QColor blue(42, 130, 218);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, darkGray);
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, black);
    darkPalette.setColor(QPalette::AlternateBase, darkGray);
    darkPalette.setColor(QPalette::ToolTipBase, blue);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, darkGray);
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::Link, blue);
    darkPalette.setColor(QPalette::Highlight, blue);
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    darkPalette.setColor(QPalette::Active, QPalette::Button, gray.darker());
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

    return darkPalette;
}
