#include "highspeedlinewidget.h"
#include "ui_highspeedlinewidget.h"

#include <QDebug>

#include "printer.h"
#include <sstream>
#include <cmath>

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

    connect(ui->printButton, &QAbstractButton::clicked, this, &HighSpeedLineWidget::print_line);

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
    print->acceleration_mm_per_s2 = ui->printAccelerationSpinBox->value();

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
    std::vector<std::string> lineCommands = print->print_commands_for_lines();

    for (auto &printLine : lineCommands)
    {
        std::stringstream s;
        s << printLine;
        emit execute_command(s);
        // wait here for user input
        // break if the user wants to stop
    }
}

std::vector<std::string> HighSpeedLineCommandGenerator::print_commands_for_lines()
{
    std::vector<std::string> returnVec;

    for (int lineNum=0; lineNum < numLines; lineNum++)
    {
        returnVec.push_back(generate_commands_for_printing_line(lineNum));
    }
    return returnVec;
}

std::string HighSpeedLineCommandGenerator::generate_commands_for_printing_line(int lineNum)
{
    std::stringstream s;

    Axis nonPrintAxis;
    if (printAxis == Axis::X) nonPrintAxis = Axis::Y;
    else nonPrintAxis == Axis::X;

    // move to the jetting position if we are not already there
    s << CMD::set_jog(Axis::X, 30);
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::begin_motion(Axis::X);
    s << CMD::motion_complete(Axis::X);

    // start PVT motion
    // position axes where they need to be for printing
    if (printAxis == Axis::Y)
    {
        // move y axis so that bed is behind the nozzle so there is enough space to get up to speed to print
        s << CMD::position_absolute(printAxis, buildBox.centerY + (print_travel_length/2.0));
        s << CMD::begin_motion(printAxis);
        s << CMD::motion_complete(printAxis);
    }

    // move the non-print axis to line position
    // need to calculate line position based on number of lines and build box size
    if (nonPrintAxis == Axis::Y)
    {
        s << CMD::set_accleration(nonPrintAxis, 600);
        s << CMD::set_deceleration(nonPrintAxis, 600);
        s << CMD::set_speed(nonPrintAxis, 60);
        s << CMD::position_absolute(nonPrintAxis, buildBox.centerY); // NEED TO OFFSET THIS FOR OTHER LINES
    }
    else
    {
        s << CMD::set_speed(nonPrintAxis, 80);
        s << CMD::position_absolute(nonPrintAxis, buildBox.centerX); // NEED TO OFFSET THIS FOR OTHER LINES
    }
    s << CMD::begin_motion(nonPrintAxis);
    s << CMD::motion_complete(nonPrintAxis);

    // if printing with the x axis
    if (printAxis == Axis::X)
    {
        s << CMD::position_absolute(printAxis, buildBox.centerX + (print_travel_length/2.0));
        s << CMD::begin_motion(printAxis);
        s << CMD::motion_complete(printAxis);
    }

    // if printing with the y axis, this was already done

    // line print move, jetting, and TTL trigger
    double print_speed_mm_per_s = (dropletSpacing_um * jettingFrequency_Hz) / 1000.0;

    int accelTimeCnts{int((print_speed_mm_per_s/acceleration_mm_per_s2)*(double)cntsPerSec)};
    int halfLinePrintTimeCnts{int((lineLength_mm / print_speed_mm_per_s) * (double)cntsPerSec / 2.0)};
    double accelDistance_mm{0.5 * acceleration_mm_per_s2 * std::pow((print_speed_mm_per_s/acceleration_mm_per_s2), 2)};

    int timeToPOICnts{accelTimeCnts + halfLinePrintTimeCnts};

    // PVT commands are in relative position coordinates

    //s << CMD::add_pvt_data_to_buffer(printAxis, accelDistance_mm, speed_mm_s, accelTimeCnts);     // accelerate
    //s << CMD::add_pvt_data_to_buffer(printAxis, lineLength_mm/2.0, speed_mm_s, halfLinePrintTimeCnts);  // constant velocity to trigger point
    //s << CMD::add_pvt_data_to_buffer(printAxis, lineLength_mm/2.0, speed_mm_s, halfLinePrintTimeCnts);  // constant velocity
    //s << CMD::add_pvt_data_to_buffer(printAxis, accelDistance_mm, 0, accelTimeCnts);     // decelerate

    //s << CMD::exit_pvt_mode(Axis::X);

    //s << CMD::begin_pvt_motion(Axis::X);

    // start line print

    // accelerate

    // trigger TTL at right time

    // start printing with gearing

    // stop gearing

    // decelerate to a stop

    // move back to jetting window

    return s.str();
}

