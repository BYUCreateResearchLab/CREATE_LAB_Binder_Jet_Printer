#include "highspeedlinewidget.h"
#include "ui_highspeedlinewidget.h"

#include <QDebug>

#include "printer.h"
#include <sstream>

HighSpeedLineWidget::HighSpeedLineWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::HighSpeedLineWidget)
{
    ui->setupUi(this);
    setAccessibleName("High Speed Line Widget");
    setup();
}

HighSpeedLineWidget::~HighSpeedLineWidget()
{
    delete ui;
}

void HighSpeedLineWidget::allow_widget_input(bool allowed)
{

}

void HighSpeedLineWidget::setup()
{
    ui->SVGViewer->setup(2, 0.5);
}

void HighSpeedLineWidget::reset_preview_zoom()
{
    ui->SVGViewer->resetZoom();
}

void HighSpeedLineWidget::print_line()
{
    std::stringstream s;

    // set speed and acceleration?

    s << CMD::position_absolute(Axis::X, 0); // move to start of print
    s << CMD::position_absolute(Axis::Y, 0);

    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);

    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);

    // add print line to buffer

    double acceleration{100.0};
    double speed_mm_s{5.0};
    double lineLength_mm{10.0};
    int TriggerOffsetTime{512};

    int cntsPerSec{1024};

    int accelTime{int((speed_mm_s/acceleration)*(double)cntsPerSec)};
    int halfLinePrintTime{int((lineLength_mm / speed_mm_s) * (double)cntsPerSec / 2.0)};
    double accelDistance{0.5 * acceleration * (speed_mm_s/acceleration) * (speed_mm_s/acceleration)};

    int timeToPOI{accelTime + halfLinePrintTime};

    qDebug() << QString::number(accelTime);

    s << CMD::add_pvt_data_to_buffer(Axis::X, accelDistance, speed_mm_s, accelTime);     // accelerate
    s << CMD::add_pvt_data_to_buffer(Axis::X, lineLength_mm/2.0, speed_mm_s, halfLinePrintTime);  // constant velocity to trigger point
    s << CMD::add_pvt_data_to_buffer(Axis::X, lineLength_mm/2.0, speed_mm_s, halfLinePrintTime);  // constant velocity
    s << CMD::add_pvt_data_to_buffer(Axis::X, accelDistance, 0, accelTime);     // decelerate

    s << CMD::exit_pvt_mode(Axis::X);

    s << CMD::set_reference_time();
    s << CMD::begin_pvt_motion(Axis::X);

    s << CMD::at_time_samples(TriggerOffsetTime); // WHAT DO I PUT HERE?

    emit execute_command(s);
}

