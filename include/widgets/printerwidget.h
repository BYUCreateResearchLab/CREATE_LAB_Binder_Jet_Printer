#ifndef PRINTERWIDGET_H
#define PRINTERWIDGET_H

#include <QWidget>
#include <sstream>
#include <QString>

#include "printer.h"
#include "printhread.h"

namespace Ui {
class PrinterWidget;
}

class PrinterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PrinterWidget(QWidget *parent = nullptr);
    virtual ~PrinterWidget();

    void pass_printer_objects(Printer *printer, PrintThread *printThread);

public slots:
    virtual void allow_widget_input(bool allowed) = 0; // =0 makes it so that every child must override this function to compile (don't put in slots in child, just public)

signals:
    void execute_command(std::stringstream &s);
    void generate_printing_message_box(const std::string &message);
    void stop_print_and_thread();
    void disable_user_input();
    void print_to_output_window(QString s);
    void jet_turned_on();
    void jet_turned_off();

protected:
    PrintThread *mPrintThread{nullptr};
    Printer *mPrinter{nullptr};
};

#endif // PRINTERWIDGET_H
