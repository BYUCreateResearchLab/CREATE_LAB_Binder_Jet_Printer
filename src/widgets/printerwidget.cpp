#include "printerwidget.h"

PrinterWidget::PrinterWidget(Printer *printer_, QWidget *parent) :
    QWidget(parent),
    mPrinter(printer_)
{
}

PrinterWidget::~PrinterWidget()
{

}

#include "moc_printerwidget.cpp"
