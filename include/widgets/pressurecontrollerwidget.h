#ifndef PRESSURECONTROLLERWIDGET_H
#define PRESSURECONTROLLERWIDGET_H

#include <QWidget>

#include "printerwidget.h"

namespace Ui {
class PressureControllerWidget;
}

class PressureControllerWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit PressureControllerWidget(Printer *printer, QWidget *parent = nullptr);
    ~PressureControllerWidget();

    void allow_widget_input(bool allowed) override;


private:
    void connect_to_pressure_controller();
    void set_pressure();
    void toggle_purge();
    void gravity_feed_up();
    void gravity_feed_down();
    void quick_purge();
    void send_command(const QString &command);
    void move_reservoir();  // MAX 03/04 !!! added command

private:
    Ui::PressureControllerWidget *ui;
};

#endif // PRESSURECONTROLLERWIDGET_H
