#include "heatlampwidget.h"
#include "ui_heatlampwidget.h"
#include "heatlamp.h"
#include "mjprintheadwidget.h"

HeatLampWidget::HeatLampWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::HeatLampWidget)
{
    ui->setupUi(this);
    setAccessibleName("Heat Lamp Widget");

    connect(ui->getBedTempButton, &QPushButton::clicked, this, &HeatLampWidget::get_bed_temp);
    connect(ui->openConnectionToControllerButton, &QPushButton::clicked, this, &HeatLampWidget::open_connection);
    connect(ui->cureLayerButton, &QPushButton::clicked, this, &HeatLampWidget::cure_layer_pressed);
    connect(ui->clearHistoryButton, &QPushButton::clicked, this, &HeatLampWidget::clear_temperature_history);
    connect(ui->setBitsButton, &QPushButton::clicked, this, &HeatLampWidget::set_bits);
    connect(ui->setIntensityButton, &QPushButton::clicked, this, &HeatLampWidget::set_intensity);
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

void HeatLampWidget::set_intensity() {
    std::stringstream ss;
    ss << mPrinter -> heatLamp -> set_intensity(ui -> intensityInput -> value());
    mPrinter -> mcu -> printerThread -> execute_command(ss);
}

void HeatLampWidget::set_bits() {
    qDebug("set_bits pressed");
    std::stringstream ss;
    if(ui -> D0Checkbox -> isChecked()) {
        qDebug("D0");
        ss << CMD::set_bit(HEATLAMP_D0);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D0);
    }
    if(ui -> D1Checkbox -> isChecked()) {
        qDebug("D1");
        ss << CMD::set_bit(HEATLAMP_D1);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D1);
    }
    if(ui -> D2Checkbox -> isChecked()) {
        qDebug("D2");
        ss << CMD::set_bit(HEATLAMP_D2);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D2);
    }
    if(ui -> D3Checkbox -> isChecked()) {
        qDebug("D3");
        ss << CMD::set_bit(HEATLAMP_D3);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D3);
    }

    mPrinter -> mcu -> printerThread -> execute_command(ss);
}

void HeatLampWidget::open_connection() {
    std::stringstream ss;
    ss << CMD::open_connection_to_controller();
    ss << CMD::detail::GCmd("CO 3");        // configures bank 2 and 3 as outputs on extended I/O (IO 17-32)
    ss << CMD::detail::GCmd("MTG=1")
       << CMD::detail::GCmd("AGG=0")
       << CMD::detail::GCmd("OFG=0")
       << CMD::detail::GCmd("TLG=5")
       << CMD::detail::GCmd("TKG=5")
       << CMD::set_bit(HEATLAMP_D0)
       << CMD::set_bit(HEATLAMP_D1)
       << CMD::set_bit(HEATLAMP_D2)
       << CMD::set_bit(HEATLAMP_D3);
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
    PrintParameters settings;
    settings.cureSpeed_mm_s = ui -> cureSpeedInput -> value();
    settings.target_temp = ui -> targetTempInput -> value();
    settings.waitAfterHeatLampOn_millisecs = ui -> waitAfterHeatLampInput -> value();
    settings.kp = ui -> kpInput -> value();
    settings.ki = ui -> kiInput -> value();

    std::stringstream s;
    s << mPrinter -> cure_layer(settings);
    mPrinter -> mcu -> printerThread -> execute_command(s);
}

#include "moc_heatlampwidget.cpp"
