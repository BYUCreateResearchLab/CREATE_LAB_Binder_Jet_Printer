#include "heatlampwidget.h"
#include "ui_heatlampwidget.h"
#include <iostream>

HeatLampWidget::HeatLampWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::HeatLampWidget)
{
    ui->setupUi(this);
    setAccessibleName("Heat Lamp Widget");

    connect(ui->getBedTempButton, &QPushButton::clicked, this, &HeatLampWidget::get_bed_temp);
}

HeatLampWidget::~HeatLampWidget()
{
    delete ui;
}

// enable/disable widgets when they should not be able to be pressed
void HeatLampWidget::allow_widget_input(bool allowed)
{
}

void HeatLampWidget::get_bed_temp() {
    std::cout << "getting temp";
    char buff[G_HUGE_BUFFER];
    ui -> text_output -> setText(QString("bed temp requested!"));
    std::stringstream ss;
    ss << CMD::read_analog_input(1);
    mPrinter -> mcu -> printerThread -> execute_command(ss);
    GMessage(mPrinter->mcu->g, buff, G_HUGE_BUFFER);
    ui -> text_output -> setText(QString(buff));
}

#include "moc_heatlampwidget.cpp"
