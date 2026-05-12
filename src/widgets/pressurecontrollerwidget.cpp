#include "pressurecontrollerwidget.h"
#include "ui_pressurecontrollerwidget.h"
#include "pcd.h"
#include "dmc4080.h"


PressureControllerWidget::PressureControllerWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::PressureControllerWidget)
{
    ui->setupUi(this);
    setAccessibleName("Pressure Controller Widget");

    connect(ui->purgeButton, &QPushButton::clicked, this, &PressureControllerWidget::toggle_purge);
    connect(ui->setPressureButton, &QPushButton::clicked, this, &PressureControllerWidget::set_pressure);
    connect(ui->quickPurgeButton, &QPushButton::clicked, this, &PressureControllerWidget::quick_purge_clicked);
    connect(ui->moveDistButton, &QPushButton::clicked, this, & PressureControllerWidget::move_reservoir);

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

void PressureControllerWidget::quick_purge_clicked()
{
    std::stringstream s;
    const int pulseTime_ms = 100; // 0.1 seconds
    s << CMD::quick_purge(pulseTime_ms); // function in printer.cpp
    emit execute_command(s);

}

void PressureControllerWidget::send_command(const QString &command)
{
    // Simple send command function TODO implement to simplify other code
    std::stringstream s;
    s << command.toStdString();
    emit execute_command(s);
}

// MAX 03/04 !!! New thing idk
void PressureControllerWidget::move_reservoir()
{
    std::stringstream s;
    double distance_mm = ui->distanceValue->value();

    // Instead of PR (Relative), use PA (Absolute)
    // You need to track the internal position or query it with _TPE
    // If you must use relative, ensure the limit is definitely set:

    s << CMD::set_speed(Axis::Reservoir, 10);
    s << CMD::position_relative(Axis::Reservoir, distance_mm);
    s << CMD::begin_motion(Axis::Reservoir);

    emit execute_command(s);
}

