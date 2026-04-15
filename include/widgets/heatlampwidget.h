#ifndef HEATLAMPWIDGET_H
#define HEATLAMPWIDGET_H

#include <QWidget>
#include <QLabel>

#include "printerwidget.h"
#include "dmc4080.h"
#include "asyncserialdevice.h"

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

    void clear_temperature_history();
    void set_intensity();
    void set_bits();
    void cure_and_roll(PrintParameters params, RecoatSettings recoatSettings);
    void print_temp_history();

private:
    void get_bed_temp();
    void open_connection();
    void cure_layer_pressed();
    void roll_and_cure_layers();
    void show_chart();
    void add_fake_temp();

private:
    Ui::HeatLampWidget *ui;
    std::vector<double> temp_history {};
};

#endif // HEATLAMPWIDGET_H
