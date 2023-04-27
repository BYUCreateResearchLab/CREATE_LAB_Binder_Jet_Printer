#include "mainwindow.h"
#include <QApplication>
#include "printer.h"

const QPalette dark_palette();

int main(int argc, char *argv[])
{
    std::unique_ptr<Printer> printer = std::make_unique<Printer>();

    QApplication a(argc, argv);
    a.setStyle("fusion");
    a.setPalette(dark_palette());

    MainWindow w(printer.get());
    w.setup();
    w.setWindowState(Qt::WindowMaximized);
    w.show();

    int ret{a.exec()};

    return ret;
}


// Dark theme settings
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
