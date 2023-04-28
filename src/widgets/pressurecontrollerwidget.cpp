#include "pressurecontrollerwidget.h"
#include "ui_pressurecontrollerwidget.h"
#include "pcd.h"

PressureControllerWidget::PressureControllerWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::PressureControllerWidget)
{
    ui->setupUi(this);
    setAccessibleName("Pressure Controller Widget");

    connect(ui->purgeButton, &QPushButton::clicked, this, &PressureControllerWidget::toggle_purge);
    connect(ui->setPressureButton, &QPushButton::clicked, this, &PressureControllerWidget::set_pressure);

    //mPrinter->pressureController->connect_to_pressure_controller();
}

PressureControllerWidget::~PressureControllerWidget()
{
    delete ui;
}

// enable/disable widgets when they should not be able to be pressed
void PressureControllerWidget::allow_widget_input(bool allowed)
{
}

void PressureControllerWidget::connect_to_pressure_controller()
{
    mPrinter->pressureController->connect_to_pressure_controller();
}

void PressureControllerWidget::set_pressure()
{
    mPrinter->pressureController->update_set_point(ui->pressureSpinBox->value());
}

void PressureControllerWidget::toggle_purge()
{
    if (ui->purgeButton->isChecked())
    {
        mPrinter->pressureController->purge();
        ui->purgeButton->setText("Stop Purge");
    }
    else
    {
        mPrinter->pressureController->stop_purge();
        ui->purgeButton->setText("Purge");
    }
}
