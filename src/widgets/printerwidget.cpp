#include "printerwidget.h"
#include "dmc4080.h"

PrinterWidget::PrinterWidget(Printer *printer_, QWidget *parent) :
    QWidget(parent),
    mPrinter(printer_)
{
    mPrintThread = mPrinter->mcu->printerThread;
}

PrinterWidget::~PrinterWidget()
{

}

#include "moc_printerwidget.cpp"
