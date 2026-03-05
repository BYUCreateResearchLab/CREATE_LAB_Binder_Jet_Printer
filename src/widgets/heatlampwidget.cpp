#include "heatlampwidget.h"
#include "ui_heatlampwidget.h"
#include "heatlamp.h"

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
    connect(ui->clearHistoryButton, &QPushButton::clicked, this, &HeatLampWidget::clear_temperature_history);
    CureSettings settings;
}

HeatLampWidget::~HeatLampWidget()
{
    delete ui;
}

// enable/disable widgets when they should not be able to be pressed
void HeatLampWidget::allow_widget_input(bool allowed)
{
}

void HeatLampWidget::clear_temperature_history() {
    if(mPrinter -> heatLamp) {
        mPrinter -> heatLamp -> clear_history();
    }
}

void HeatLampWidget::open_connection() {
    std::stringstream ss;
    ss << CMD::open_connection_to_controller();
    ss << CMD::detail::GCmd("MTE=1")
       << CMD::detail::GCmd("AGE=0")
       << CMD::detail::GCmd("OFE=-10");
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
    settings.kp = ui -> kpInput -> value();
    settings.ki = ui -> kiInput -> value();

    std::stringstream s;
    s << mPrinter -> cure_layer(settings);
    mPrinter -> mcu -> printerThread -> execute_command(s);
}

#include "moc_heatlampwidget.cpp"
