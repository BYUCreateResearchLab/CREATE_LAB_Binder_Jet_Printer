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
    connect(ui->quickPurgeButton, &QPushButton::clicked, this, &PressureControllerWidget::quick_purge);
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

void PressureControllerWidget::quick_purge()
{
    //TO DO Add purge for a fraction of a second

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

    // Get the actual value from the UI (removing the hardcoded '3')
    double distance_mm = ui->distanceValue->value();
    double speed_mm_s = 5;

    // Build the command string using the same pattern as MJPrintheadWidget::on_xHome_clicked_MJ
    s << CMD::set_accleration(Axis::Reservoir, 200);
    s << CMD::set_deceleration(Axis::Reservoir, 200);
    s << CMD::set_speed(Axis::Reservoir, speed_mm_s);
    s << CMD::position_relative(Axis::Reservoir, distance_mm);
    s << CMD::begin_motion(Axis::Reservoir);

    // This sends the commands line-by-line via the established signal
    emit execute_command(s);

    // std::stringstream s;

    // double distance_mm = ui->distanceValue->value();
    // distance_mm = 3;
    // double speed_mm_s = 5;

    // // Set the movement params
    // s << CMD::set_accleration(Axis::Reservoir, 200);     // mm/s^2
    // s << CMD::set_deceleration(Axis::Reservoir, 200);    // mm/s^2
    // s << CMD::set_speed(Axis::Reservoir, speed_mm_s);   // mm/s

    // // Command the movement
    // s << CMD::position_relative(Axis::Reservoir, distance_mm);
    // s << CMD::begin_motion(Axis::Reservoir);
    // s << CMD::after_motion(Axis::Reservoir);

    // // Convert stringstream to CMD formatted string
    // std::string dmc_commands = CMD::cmd_buf_to_dmc(s);
    // const char *cmds = dmc_commands.c_str();

    // if (mPrinter->mcu->g) {
    //     GProgramDownload(mPrinter->mcu->g, cmds, "");
    // }

    // std::stringstream c_cmdMove;
    // c_cmdMove << "GCmd," << "XQ" << "\n";
    // c_cmdMove << "GProgramComplete," << "\n";

    // emit execute_command(c_cmdMove);

}

#include "moc_pressurecontrollerwidget.cpp"
