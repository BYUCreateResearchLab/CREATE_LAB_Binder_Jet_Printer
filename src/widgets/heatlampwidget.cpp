#include "heatlampwidget.h"
#include "ui_heatlampwidget.h"

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
    ui -> text_output -> setText(QString("bed temp requested!"));
}

#include "moc_heatlampwidget.cpp"
