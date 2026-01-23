#ifndef HEATLAMPWIDGET_H
#define HEATLAMPWIDGET_H

#include <QWidget>
#include <QLabel>

#include "printerwidget.h"

namespace Ui {
class HeatLampWidget;
}

class HeatLampWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit HeatLampWidget(Printer *printer, QWidget *parent = nullptr);
    ~HeatLampWidget();

    void allow_widget_input(bool allowed) override;


private:
    void get_bed_temp();

private:
    Ui::PressureControllerWidget *ui;
};

#endif // HEATLAMPWIDGET_H
