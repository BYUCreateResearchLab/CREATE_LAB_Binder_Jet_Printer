#include "highspeedlinewidget.h"
#include "ui_highspeedlinewidget.h"

#include <QDebug>

#include "printer.h"
#include <sstream>

HighSpeedLineWidget::HighSpeedLineWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::HighSpeedLineWidget)
{
    ui->setupUi(this);
    setAccessibleName("High Speed Line Widget");
    print = new HighSpeedLineCommandGenerator{};
    setup();
}

HighSpeedLineWidget::~HighSpeedLineWidget()
{
    delete print;
    delete ui;
}

void HighSpeedLineWidget::allow_widget_input(bool allowed)
{

}

void HighSpeedLineWidget::setup()
{
    // Connect all Spin Boxes to update print settings when finished editing
    QList<QAbstractSpinBox *> printSettingWidgets = this->findChildren<QAbstractSpinBox *> ();
    for(int i{0}; i < printSettingWidgets.count(); ++i)
    {
       connect(printSettingWidgets[i], &QAbstractSpinBox::editingFinished, this, &HighSpeedLineWidget::update_print_settings);
    }

    // Connect all QComboBoxes to update print settings when index is changed
    QList<QComboBox *> printSettingWidgets2 = this->findChildren<QComboBox *> ();
    for(int i{0}; i < printSettingWidgets2.count(); ++i)
    {
       connect(printSettingWidgets2[i], qOverload<int>(&QComboBox::currentIndexChanged), this, &HighSpeedLineWidget::update_print_settings);
    }

    update_print_settings();
}

void HighSpeedLineWidget::update_print_settings()
{
    // check for if other things need to be updated
    bool mustUpdatePreviewWindow{false};
    Axis currentViewAxis = print->viewAxis;
    Axis newViewAxis = ui->viewAxisComboBox->currentIndex() == 0 ? Axis::X : Axis::Y;

    // check to see if build box stats changed and update the preview window if needed
    if (currentViewAxis != newViewAxis) mustUpdatePreviewWindow = true;
    else if (print->buildBox.length != ui->buildBoxLengthSpinBox->value()) mustUpdatePreviewWindow = true;
    else if (print->buildBox.thickness != ui->buildBoxThicknessSpinBox->value()) mustUpdatePreviewWindow = true;

    // update print parameters
    print->numLines = ui->numLinesSpinBox->value();
    print->lineSpacing_um = ui->lineSpacingSpinBox->value();
    print->lineLength_mm = ui->lineLengthSpinBox->value();
    print->dropletSpacing_um = ui->dropletSpacingSpinBox->value();
    print->jettingFrequency_Hz = ui->jettingFrequencySpinBox->value();
    print->acceleration_mm_per_s = ui->printAccelerationSpinBox->value();

    // update build box parameters
    print->buildBox.centerX = ui->buildBoxCenterXSpinBox->value();
    print->buildBox.centerY = ui->buildBoxCenterYSpinBox->value();
    print->buildBox.thickness = ui->buildBoxThicknessSpinBox->value();
    print->buildBox.length = ui->buildBoxLengthSpinBox->value();

    // update observation settings - first item in box should be x, second is y
    print->printAxis = ui->printAxisComboBox->currentIndex() == 0 ? Axis::X : Axis::Y;
    print->viewAxis = ui->viewAxisComboBox->currentIndex() == 0 ? Axis::X : Axis::Y;
    print->triggerOffset_ms =ui->triggerOffsetSpinBox->value();

    // disable the line spacing spin box if there is only one line being printed
    if (print->numLines == 1) ui->lineSpacingSpinBox->setEnabled(false);
    else ui->lineSpacingSpinBox->setEnabled(true);

    if (mustUpdatePreviewWindow)
    {
        if (print->viewAxis == Axis::Y)
        {
            ui->SVGViewer->setup(print->buildBox.length, print->buildBox.thickness);
            qDebug() << print->buildBox.length;
            qDebug() << print->buildBox.thickness;
        }
        else
        {
            ui->SVGViewer->setup(print->buildBox.thickness, print->buildBox.length);
        }
    }
}

// this is just here for the QComboBoxes to be able to update print settings
void HighSpeedLineWidget::update_print_axes(int index)
{
    update_print_settings();
}

void HighSpeedLineWidget::reset_preview_zoom()
{
    ui->SVGViewer->resetZoom();
}

void HighSpeedLineWidget::print_line()
{
    std::vector<std::stringstream> lineCommands;

    for (auto &printLine : lineCommands)
    {
        emit execute_command(printLine);
        // wait here for user input
        // break if the user wants to stop
    }
}

std::vector<std::string> HighSpeedLineCommandGenerator::print_commands_for_lines()
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

    std::vector<std::string> returnVec;
    returnVec.push_back(s.str());
    return returnVec;
}

