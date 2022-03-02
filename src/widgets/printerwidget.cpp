#include "printerwidget.h"

PrinterWidget::PrinterWidget(QWidget *parent) : QWidget(parent)
{

}

PrinterWidget::~PrinterWidget()
{

}

void PrinterWidget::pass_printer_objects(Printer *printer, PrintThread *printThread)
{
    mPrinter = printer;
    mPrintThread = printThread;
}

