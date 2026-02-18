#include "heatlampwidget.h"
#include "ui_heatlampwidget.h"

HeatLampWidget::HeatLampWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::HeatLampWidget)
{
    ui->setupUi(this);
    setAccessibleName("Heat Lamp Widget");

    connect(ui->getBedTempButton, &QPushButton::clicked, this, &HeatLampWidget::get_bed_temp);
    connect(ui->openConnectionToControllerButton, &QPushButton::clicked, this, &HeatLampWidget::open_connection);
    connect(ui->cureLayerButton, &QPushButton::clicked, this, &HeatLampWidget::cure_layer_pressed);
    connect(ui->setVoltageButton, &QPushButton::clicked, this, &HeatLampWidget::set_voltage);
    CureSettings settings;
    // ui -> cureSpeedInput -> setValue(settings.cureSpeed_mm_s);
    // ui -> heatLampEndInput -> setValue(settings.heatLampEnd_mm);
    // ui -> heatLampStartInput -> setValue(settings.heatLampStart_mm);
    // ui -> pyrometerPositionInput -> setValue(settings.pyrometerPosition_mm);
    // ui -> targetTempInput -> setValue(settings.target_temp);
    // ui -> waitAfterHeatLampInput -> setValue(settings.waitAfterHeatLampOn_millisecs);
    // ui -> traverseSpeedInput -> setValue(settings.yAxisTraverseSpeed_mm_s);
}

HeatLampWidget::~HeatLampWidget()
{
    delete ui;
}

// enable/disable widgets when they should not be able to be pressed
void HeatLampWidget::allow_widget_input(bool allowed)
{
}

void HeatLampWidget::open_connection() {
    std::stringstream ss;
    ss << CMD::open_connection_to_controller();
    ss << CMD::detail::GCmd("MTE=1")
       << CMD::detail::GCmd("AGE=0")
       << CMD::detail::GCmd("TLE=5")
       << CMD::detail::GCmd("TKE=5")
       << CMD::detail::GCmd("OFE=0");
    mPrinter -> mcu -> printerThread -> execute_command(ss);
}

void HeatLampWidget::set_voltage() {
    std::stringstream ss;
    ss << CMD::offset(Axis::HeatLamp, ui -> voltageInput->value());
    ss << CMD::servo_here(Axis::HeatLamp);

    mPrinter -> mcu -> printerThread -> execute_command(ss);
}

void HeatLampWidget::get_bed_temp() {
    char buff[G_SMALL_BUFFER];
    double temp;
    ui -> text_output -> setText(QString("bed temp requested!"));
    std::stringstream ss;
    ss << CMD::deallocate_array("BEDTEMP");
    ss << CMD::define_array("BEDTEMP", 1);
    ss << CMD::detail::GCmd() + "BEDTEMP[0] = @AN[1] \n";
    mPrinter -> mcu -> printerThread -> execute_command(ss);
    for(int i = 0; i < 10000000; i++){}
    if(mPrinter->mcu->g) {
        GArrayUpload(mPrinter->mcu->g, "BEDTEMP", 0, 0, G_COMMA, buff, G_SMALL_BUFFER);
        temp = std::stod(buff);
        ui -> text_output -> setText(QString::number(temp));
    } else {
        qDebug("controller not connected");
    }
}

void HeatLampWidget::cure_layer_pressed() {
    CureSettings settings;
    settings.cureSpeed_mm_s = ui -> cureSpeedInput -> value();
    settings.heatLampEnd_mm = ui -> heatLampEndInput -> value();
    settings.heatLampStart_mm = ui -> heatLampStartInput -> value();
    settings.pyrometerPosition_mm = ui -> pyrometerPositionInput -> value();
    settings.target_temp = ui -> targetTempInput -> value();
    settings.waitAfterHeatLampOn_millisecs = ui -> waitAfterHeatLampInput -> value();
    settings.yAxisTraverseSpeed_mm_s = ui -> traverseSpeedInput -> value();

    std::stringstream s;
    s << mPrinter -> cure_layer(settings);
    mPrinter -> mcu -> printerThread -> execute_command(s);
}

#include "moc_heatlampwidget.cpp"
