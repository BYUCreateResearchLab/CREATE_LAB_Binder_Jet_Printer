#ifndef PRINTERWIDGET_H
#define PRINTERWIDGET_H

#include <QWidget>
#include <sstream>

namespace Ui {
class PrinterWidget;
}

class PrinterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PrinterWidget(QWidget *parent = nullptr);
    virtual ~PrinterWidget();

public slots:
    virtual void allow_widget_input(bool allowed) = 0; // =0 makes it so that every child must override this function to compile (don't put in slots in child, just public)

signals:
    void execute_command(std::stringstream &s);
    void generate_printing_message_box(const std::string &message);
    void disable_user_input();
};

#endif // PRINTERWIDGET_H
