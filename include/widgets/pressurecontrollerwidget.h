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

private:
    Ui::PressureControllerWidget *ui;
};

#endif // PRESSURECONTROLLERWIDGET_H
