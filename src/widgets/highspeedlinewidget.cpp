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
    //allow_user_to_change_parameters(allowed);
    ui->printButton->setEnabled(allowed);

    ui->moveToCenterButton->setEnabled(allowed);
    ui->setXCenter->setEnabled(allowed);
    ui->setYCenter->setEnabled(allowed);
    ui->recordFlatButton->setEnabled(allowed);
}

void HighSpeedLineWidget::allow_user_to_change_parameters(bool allowed)
{
    ui->printParametersFrame->setEnabled(allowed);
    ui->buildBoxParametersFrame->setEnabled(allowed);
    ui->observationSettingsFrame->setEnabled(allowed);
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
    connect(ui->stopPrintButton, &QAbstractButton::clicked, this, &HighSpeedLineWidget::stop_printing);

    connect(ui->moveToCenterButton, &QAbstractButton::clicked, this, &HighSpeedLineWidget::move_to_build_box_center);

    connect(ui->setXCenter, &QAbstractButton::clicked, this, &HighSpeedLineWidget::set_x_center);
    connect(ui->setYCenter, &QAbstractButton::clicked, this, &HighSpeedLineWidget::set_y_center);

    connect(ui->recordFlatButton, &QAbstractButton::clicked, this, &HighSpeedLineWidget::view_flat);

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

void HighSpeedLineWidget::set_x_center()
{
    int currentXPos;
    GCmdI(mPrinter->g, "TPX", &currentXPos);
    double currentXPos_mm = currentXPos / (double)X_CNTS_PER_MM;
    ui->buildBoxCenterXSpinBox->setValue(currentXPos_mm);
    update_print_settings();
}

void HighSpeedLineWidget::set_y_center()
{
    int currentYPos;
    GCmdI(mPrinter->g, "TPY", &currentYPos);
    double currentYPos_mm = currentYPos / (double)Y_CNTS_PER_MM;
    ui->buildBoxCenterYSpinBox->setValue(currentYPos_mm);
    update_print_settings();
}

void HighSpeedLineWidget::reset_preview_zoom()
{
    ui->SVGViewer->resetZoom();
}

void HighSpeedLineWidget::print_line()
{

    std::stringstream s;
    std::string temp = print->generate_dmc_commands_for_printing_line(currentLineToPrintIndex);
    const char *commands = temp.c_str();

    qDebug().noquote() << commands;

    //qDebug().noquote() << commands;

    if (mPrinter->g)
    {
        GProgramDownload(mPrinter->g, commands, "");
    }
    s << "GCmd," << "XQ" << "\n";
    s << "GProgramComplete," << "\n";

    std::string linePrintMessage = "Printing Line " + std::to_string(currentLineToPrintIndex + 1);
    emit print_to_output_window(QString::fromStdString(linePrintMessage));


    allow_user_to_change_parameters(false);

    emit execute_command(s);
    emit jet_turned_on();
    emit disable_user_input();
    ui->stopPrintButton->setEnabled(true);
    printIsRunning_ = true;
    ui->stopPrintButton->setText("\nStop Printing\n");
    connect(mPrintThread, &PrintThread::ended, this, &HighSpeedLineWidget::when_line_print_completed);
}

void HighSpeedLineWidget::view_flat()
{
    std::stringstream s;
    std::string temp = print->generate_dmc_commands_for_viewing_flat(currentLineToPrintIndex);
    const char *commands = temp.c_str();

    if (mPrinter->g)
    {
        GProgramDownload(mPrinter->g, commands, "");
    }
    s << "GCmd," << "XQ" << "\n";
    s << "GProgramComplete," << "\n";

    std::string linePrintMessage = "Viewing flat for line " + std::to_string(currentLineToPrintIndex + 1);
    emit print_to_output_window(QString::fromStdString(linePrintMessage));

    emit execute_command(s);
    emit jet_turned_on();
    //emit disable_user_input();
    //ui->stopPrintButton->setEnabled(true);
    //printIsRunning_ = true;
    //ui->stopPrintButton->setText("\nStop Printing\n");
}

void HighSpeedLineWidget::when_line_print_completed()
{
    disconnect(mPrintThread, &PrintThread::ended, this, &HighSpeedLineWidget::when_line_print_completed); // run once
    currentLineToPrintIndex++;

    printIsRunning_ = false;
    ui->stopPrintButton->setText("\nReset Print\n");

    if (currentLineToPrintIndex < print->numLines)
    {
        ui->printButton->setText(QString("\nPrint Line ") + QString::number(currentLineToPrintIndex + 1) + QString("\n"));
    }

    if (currentLineToPrintIndex >= (print->numLines)) // the print is done
    {
        reset_print();
    }
}

void HighSpeedLineWidget::stop_printing()
{
    if (printIsRunning_)
    {
        disconnect(mPrintThread, &PrintThread::ended, this, &HighSpeedLineWidget::when_line_print_completed);
        emit stop_print_and_thread();
        emit jet_turned_off();
        emit print_to_output_window("Print Stopped");

        printIsRunning_ = false;
        ui->stopPrintButton->setText("\nReset Print\n");
        if (currentLineToPrintIndex == 0)
        {
            reset_print();
        }

    }
    else if (currentLineToPrintIndex != 0)
    {
        if (currentLineToPrintIndex < print->numLines)
            emit print_to_output_window("Print ended early");
        else
            emit print_to_output_window("All lines have been printed");

        reset_print();
    }
}

void HighSpeedLineWidget::reset_print()
{
    allow_user_to_change_parameters(true);
    ui->stopPrintButton->setEnabled(false);
    ui->printButton->setText("\nStart Print\n");
    currentLineToPrintIndex = 0; // reset line index counter
}

std::string HighSpeedLineCommandGenerator::generate_commands_for_printing_line(int lineNum)
{
    std::stringstream s;

    Axis nonPrintAxis;
    if (printAxis == Axis::X) nonPrintAxis = Axis::Y;
    else nonPrintAxis = Axis::X;

    // line print move, jetting, and TTL trigger
    double print_speed_mm_per_s = (dropletSpacing_um * jettingFrequency_Hz) / 1000.0;

    double accelTime = print_speed_mm_per_s/acceleration_mm_per_s2;
    int accelTimeCnts{int((accelTime) * (double)cntsPerSec)};
    double linePrintTime_s = (lineLength_mm / print_speed_mm_per_s);
    int halfLinePrintTimeCnts{int((linePrintTime_s * (double)cntsPerSec) / 2.0)};
    double accelDistance_mm{0.5 * acceleration_mm_per_s2 * std::pow(accelTime, 2)};

    std::string linePrintMessage = "Printing Line " + std::to_string(lineNum + 1);
    s << CMD::display_message(linePrintMessage);

    // move to the jetting position if we are not already there
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::set_speed(Axis::X, xTravelSpeed);
    s << CMD::position_absolute(Axis::X, X_STAGE_LEN_MM);
    s << CMD::begin_motion(Axis::X);
    s << CMD::motion_complete(Axis::X);

    // start PVT motion
    // position axes where they need to be for printing
    if (printAxis == Axis::Y)
    {
        // move y axis so that bed is behind the nozzle so there is enough space to get up to speed to print
        s << CMD::set_speed(printAxis, 60);
        s << CMD::position_absolute(printAxis, buildBox.centerY + (lineLength_mm/2.0) + accelDistance_mm);
        s << CMD::begin_motion(printAxis);
        s << CMD::motion_complete(printAxis);
    }

    // move the non-print axis to line position
    if (nonPrintAxis == Axis::Y)
    {
        s << CMD::set_accleration(nonPrintAxis, 600);
        s << CMD::set_deceleration(nonPrintAxis, 600);
        s << CMD::set_speed(nonPrintAxis, 60);

        double layersize = (numLines-1)*(lineSpacing_um / 1000.0);
        double initialOffset = buildBox.centerY - (layersize/2.0);

        s << CMD::position_absolute(nonPrintAxis, initialOffset + (lineNum*(lineSpacing_um/1000.0)));
    }
    else
    {
        s << CMD::set_speed(nonPrintAxis, xTravelSpeed);

        double layersize = (numLines-1)*(lineSpacing_um / 1000.0);
        double initialOffset = buildBox.centerX + (layersize / 2.0);
        s << CMD::position_absolute(nonPrintAxis, initialOffset - (lineNum*(lineSpacing_um/1000.0)));
    }

    s << CMD::begin_motion(nonPrintAxis);
    s << CMD::motion_complete(nonPrintAxis);

    // setup jetting axis
    s << CMD::stop_motion(Axis::Jet); // stop jetting if previously jetting
    s << CMD::servo_here(Axis::Jet);
    s << CMD::set_accleration(Axis::Jet, 20000000); // set super high acceleration for jetting axis


    // if printing with the x axis
    if (printAxis == Axis::X)
    {
        s << CMD::set_speed(printAxis, xTravelSpeed);
        s << CMD::position_absolute(printAxis, buildBox.centerX + (lineLength_mm/2.0) + accelDistance_mm);
        s << CMD::begin_motion(printAxis);
        s << CMD::motion_complete(printAxis);
    }
    // if printing with the y axis, this was already done

    // Line Print PVT Commands
    // PVT commands are in relative position coordinates
    s << CMD::enable_gearing_for(Axis::Jet, printAxis);
    s << CMD::add_pvt_data_to_buffer(printAxis, -accelDistance_mm,    -print_speed_mm_per_s, accelTimeCnts);         // accelerate
    s << CMD::add_pvt_data_to_buffer(printAxis, -(lineLength_mm/2.0), -print_speed_mm_per_s, halfLinePrintTimeCnts); // constant velocity to trigger point
    s << CMD::add_pvt_data_to_buffer(printAxis, -(lineLength_mm/2.0), -print_speed_mm_per_s, halfLinePrintTimeCnts); // constant velocity
    s << CMD::add_pvt_data_to_buffer(printAxis, -accelDistance_mm,     0,                    accelTimeCnts);         // decelerate
    s << CMD::exit_pvt_mode(printAxis);

    double linePrintTime_ms = linePrintTime_s * 1000.0;
    double halfLinePrintTime_ms = linePrintTime_ms / 2.0;
    double accelTime_ms = accelTime * 1000.0;
    if (triggerOffset_ms < halfLinePrintTime_ms) // trigger occurs during line print
    {
        s << CMD::display_message("Trigger occured during line print");
        s << CMD::begin_pvt_motion(printAxis);
        s << CMD::sleep(accelTime_ms);
        s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
        s << CMD::sleep(halfLinePrintTime_ms - triggerOffset_ms);
        s << CMD::set_bit(HS_TTL_BIT);
        s << CMD::sleep(triggerOffset_ms + halfLinePrintTime_ms);
    }
    else if (triggerOffset_ms < (halfLinePrintTime_ms + accelTime_ms)) // trigger occurs during acceleration
    {
        s << CMD::display_message("Trigger occured during acceleration");
        s << CMD::begin_pvt_motion(printAxis);
        s << CMD::sleep(accelTime_ms + halfLinePrintTime_ms - triggerOffset_ms);
        s << CMD::set_bit(HS_TTL_BIT);
        s << CMD::sleep(triggerOffset_ms - halfLinePrintTime_ms);
        s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
        s << CMD::sleep(linePrintTime_ms);
    }
    else // trigger occurs before acceleration
    {
        s << CMD::display_message("Trigger occured before acceleration");
        s << CMD::set_bit(HS_TTL_BIT);
        s << CMD::sleep(triggerOffset_ms - accelTime_ms - halfLinePrintTime_ms);
        s << CMD::begin_pvt_motion(printAxis);
        s << CMD::sleep(accelTime_ms);
        s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
        s << CMD::sleep(linePrintTime_ms);
    }

    s << CMD::disable_gearing_for(Axis::Jet);
    s << CMD::motion_complete(printAxis);
    s << CMD::clear_bit(HS_TTL_BIT);

    // move x-axis back to jetting position
    s << CMD::set_speed(Axis::X, xTravelSpeed);
    s << CMD::position_absolute(Axis::X, X_STAGE_LEN_MM);
    s << CMD::begin_motion(Axis::X);
    s << CMD::motion_complete(Axis::X);

    s << CMD::display_message("Line printed");

    s << CMD::set_jog(Axis::Jet, 1000); // jet at 1000z while waiting
    s << CMD::begin_motion(Axis::Jet);

    return s.str();
}

void HighSpeedLineWidget::move_to_build_box_center()
{
    std::stringstream s;
    s << CMD::display_message("Moving to build box center");
    s << CMD::set_speed(Axis::X, 60);
    s << CMD::set_speed(Axis::Y, 40);
    s << CMD::position_absolute(Axis::X, print->buildBox.centerX);
    s << CMD::position_absolute(Axis::Y, print->buildBox.centerY);
    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);

    emit execute_command(s);
    emit disable_user_input();
}

std::string HighSpeedLineCommandGenerator::generate_dmc_commands_for_printing_line(int lineNum)
{
    std::stringstream s;

    Axis nonPrintAxis;
    if (printAxis == Axis::X) nonPrintAxis = Axis::Y;
    else nonPrintAxis = Axis::X;

    // line print move, jetting, and TTL trigger
    double print_speed_mm_per_s = (dropletSpacing_um * jettingFrequency_Hz) / 1000.0;

    double accelTime = print_speed_mm_per_s/acceleration_mm_per_s2;
    int accelTimeCnts = int(std::round((accelTime) * (double)cntsPerSec));
    double linePrintTime_s = (lineLength_mm / print_speed_mm_per_s);
    int halfLinePrintTimeCnts = int(std::round((linePrintTime_s * (double)cntsPerSec) / 2.0));
    double accelDistance_mm{0.5 * acceleration_mm_per_s2 * std::pow(accelTime, 2)};

    // move to the jetting position if we are not already there
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::set_speed(Axis::X, xTravelSpeed);
    s << CMD::position_absolute(Axis::X, X_STAGE_LEN_MM);
    s << CMD::begin_motion(Axis::X);
    s << CMD::after_motion(Axis::X);

    // start PVT motion
    // position axes where they need to be for printing
    if (printAxis == Axis::Y)
    {
        // move y axis so that bed is behind the nozzle so there is enough space to get up to speed to print
        s << CMD::set_speed(printAxis, 60);
        s << CMD::position_absolute(printAxis, buildBox.centerY + (lineLength_mm/2.0) + accelDistance_mm);
        s << CMD::begin_motion(printAxis);
        s << CMD::after_motion(printAxis);
    }

    // move the non-print axis to line position
    if (nonPrintAxis == Axis::Y)
    {
        s << CMD::set_accleration(nonPrintAxis, 600);
        s << CMD::set_deceleration(nonPrintAxis, 600);
        s << CMD::set_speed(nonPrintAxis, 60);

        double layersize = (numLines-1)*(lineSpacing_um / 1000.0);
        double initialOffset = buildBox.centerY - (layersize/2.0);

        s << CMD::position_absolute(nonPrintAxis, initialOffset + (lineNum*(lineSpacing_um/1000.0)));
    }
    else
    {
        s << CMD::set_speed(nonPrintAxis, xTravelSpeed);

        double layersize = (numLines-1)*(lineSpacing_um / 1000.0);
        double initialOffset = buildBox.centerX + (layersize / 2.0);
        s << CMD::position_absolute(nonPrintAxis, initialOffset - (lineNum*(lineSpacing_um/1000.0)));
    }

    s << CMD::begin_motion(nonPrintAxis);
    s << CMD::after_motion(nonPrintAxis);

    // setup jetting axis
    s << CMD::stop_motion(Axis::Jet); // stop jetting if previously jetting
    //s << CMD::servo_here(Axis::Jet);
    //s << CMD::set_accleration(Axis::Jet, 20000000); // set super high acceleration for jetting axis

    s << CMD::enable_gearing_for(Axis::Jet, printAxis);
    // continuous jetting (don't turn off before printing the line)
    s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);


    // if printing with the x axis
    if (printAxis == Axis::X)
    {
        s << CMD::set_speed(printAxis, xTravelSpeed);
        s << CMD::position_absolute(printAxis, buildBox.centerX + (lineLength_mm/2.0) + accelDistance_mm);
        s << CMD::begin_motion(printAxis);
        s << CMD::after_motion(printAxis);
    }
    // if printing with the y axis, this was already done

    // Line Print PVT Commands
    // PVT commands are in relative position coordinates

    s << CMD::add_pvt_data_to_buffer(printAxis, -accelDistance_mm,    -print_speed_mm_per_s, accelTimeCnts);         // accelerate
    s << CMD::add_pvt_data_to_buffer(printAxis, -(lineLength_mm/2.0), -print_speed_mm_per_s, halfLinePrintTimeCnts); // constant velocity to trigger point
    s << CMD::add_pvt_data_to_buffer(printAxis, -(lineLength_mm/2.0), -print_speed_mm_per_s, halfLinePrintTimeCnts); // constant velocity
    s << CMD::add_pvt_data_to_buffer(printAxis, -accelDistance_mm,     0,                    accelTimeCnts);         // decelerate
    s << CMD::exit_pvt_mode(printAxis);
    s << CMD::set_reference_time();



    double linePrintTime_ms = linePrintTime_s * 1000.0;
    double halfLinePrintTime_ms = linePrintTime_ms / 2.0;
    double accelTime_ms = accelTime * 1000.0;
    if (triggerOffset_ms < halfLinePrintTime_ms) // trigger occurs during line print
    {
        qDebug() << "trigger occured during line print";

        //int time1 = std::round(accelTime_ms);
        //int time2 = std::round(halfLinePrintTime_ms - triggerOffset_ms);
        //int time3 = std::round(triggerOffset_ms + halfLinePrintTime_ms);
        //if (time1 != 0) s << CMD::wait(time1);
        //if (time2 != 0) s << CMD::wait(time2);
        //if (time3 != 0) s << CMD::wait(time3);

        int time1 = std::round(accelTime_ms);
        int time2 = std::round(accelTime_ms + halfLinePrintTime_ms - triggerOffset_ms);
        int time3 = std::round(accelTime_ms + linePrintTime_ms);

        s << CMD::begin_pvt_motion(printAxis);
        s << CMD::at_time_milliseconds(time1);
        //s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
        if (time2 != time1) s << CMD::at_time_milliseconds(time2);
        s << CMD::set_bit(HS_TTL_BIT);
        s << CMD::at_time_milliseconds(time3);
    }
    else if (triggerOffset_ms < (halfLinePrintTime_ms + accelTime_ms)) // trigger occurs during acceleration
    {
        qDebug() << "trigger occured during acceleration";

        //int time1 = std::round(accelTime_ms + halfLinePrintTime_ms - triggerOffset_ms);
        //int time2 = std::round(triggerOffset_ms - halfLinePrintTime_ms);
        //int time3 = std::round(linePrintTime_ms);
        //if (time1 != 0) s << CMD::wait(time1);
        //if (time2 != 0) s << CMD::wait(time2);
        //if (time3 != 0) s << CMD::wait(time3);

        int time1 = std::round(accelTime_ms + halfLinePrintTime_ms - triggerOffset_ms);
        int time2 = std::round(accelTime_ms);
        int time3 = std::round(accelTime_ms + linePrintTime_ms);

        s << CMD::begin_pvt_motion(printAxis);
        if (time1 != 0) s << CMD::at_time_milliseconds(time1);
        s << CMD::set_bit(HS_TTL_BIT);
        if (time2 != time1) s << CMD::at_time_milliseconds(time2);
        //s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
        s << CMD::at_time_milliseconds(time3);
    }
    else // trigger occurs before acceleration
    {
        qDebug() << "trigger occured before acceleration";

        //int time1 = std::round(triggerOffset_ms - accelTime_ms - halfLinePrintTime_ms);
        //int time2 = std::round(accelTime_ms);
        //int time3 = std::round(linePrintTime_ms);
        //if (time1 != 0) s << CMD::wait(time1);
        //if (time2 != 0) s << CMD::wait(time2);
        //if (time3 != 0) s << CMD::wait(time3);

        int timeWaitBeforePrint = triggerOffset_ms - accelTime_ms - halfLinePrintTime_ms;
        int time1 = std::round(timeWaitBeforePrint);
        int time2 = std::round(timeWaitBeforePrint + accelTime_ms);
        int time3 = std::round(timeWaitBeforePrint + accelTime_ms + linePrintTime_ms);

        s << CMD::set_bit(HS_TTL_BIT);
        if (time1 != 0) s << CMD::at_time_milliseconds(time1);
        s << CMD::begin_pvt_motion(printAxis);
        if (time2 != time1) s << CMD::at_time_milliseconds(time2);
        //s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
        s << CMD::at_time_milliseconds(time3);
    }

    s << CMD::disable_gearing_for(Axis::Jet);
    s << CMD::after_motion(printAxis);
    s << CMD::clear_bit(HS_TTL_BIT);

    // move x-axis back to jetting position
    s << CMD::set_speed(Axis::X, xTravelSpeed);
    s << CMD::position_absolute(Axis::X, X_STAGE_LEN_MM);
    s << CMD::begin_motion(Axis::X);
    s << CMD::after_motion(Axis::X);

    s << CMD::set_jog(Axis::Jet, 1000); // jet at 1000z while waiting
    s << CMD::begin_motion(Axis::Jet);

    std::string returnString = CMD::cmd_buf_to_dmc(s);

    return returnString;
}

std::string HighSpeedLineCommandGenerator::generate_dmc_commands_for_viewing_flat(int lineNum)
{
    std::stringstream s;

    Axis nonPrintAxis;
    if (printAxis == Axis::X) nonPrintAxis = Axis::Y;
    else nonPrintAxis = Axis::X;

    // line print move, jetting, and TTL trigger
    double print_speed_mm_per_s = (dropletSpacing_um * jettingFrequency_Hz) / 1000.0;

    double accelTime = print_speed_mm_per_s/acceleration_mm_per_s2;
    int accelTimeCnts = int(std::round((accelTime) * (double)cntsPerSec));
    double linePrintTime_s = (lineLength_mm / print_speed_mm_per_s);
    int halfLinePrintTimeCnts = int(std::round((linePrintTime_s * (double)cntsPerSec) / 2.0));
    double accelDistance_mm{0.5 * acceleration_mm_per_s2 * std::pow(accelTime, 2)};

    // start PVT motion
    // position axes where they need to be for printing
    if (printAxis == Axis::Y)
    {
        // move y axis so that bed is behind the nozzle so there is enough space to get up to speed to print
        s << CMD::set_speed(printAxis, 60);
        s << CMD::position_absolute(printAxis, buildBox.centerY + (lineLength_mm/2.0) + accelDistance_mm);
        s << CMD::begin_motion(printAxis);
        s << CMD::after_motion(printAxis);
    }

    // move the non-print axis to line position
    if (nonPrintAxis == Axis::Y)
    {
        s << CMD::set_accleration(nonPrintAxis, 600);
        s << CMD::set_deceleration(nonPrintAxis, 600);
        s << CMD::set_speed(nonPrintAxis, 60);

        double layersize = (numLines-1)*(lineSpacing_um / 1000.0);
        double initialOffset = buildBox.centerY - (layersize/2.0);

        s << CMD::position_absolute(nonPrintAxis, initialOffset + (lineNum*(lineSpacing_um/1000.0)));
    }

    s << CMD::begin_motion(nonPrintAxis);
    s << CMD::after_motion(nonPrintAxis);

    // if printing with the y axis, this was already done

    // setup jetting axis
    s << CMD::stop_motion(Axis::Jet); // stop jetting if previously jetting
    s << CMD::servo_here(Axis::Jet);
    s << CMD::set_accleration(Axis::Jet, 20000000); // set super high acceleration for jetting axis

    if (printAxis == Axis::Y) // only move if we are printing with the y-axis
    {

        // Line Print PVT Commands
        // PVT commands are in relative position coordinates
        s << CMD::enable_gearing_for(Axis::Jet, printAxis);
        s << CMD::add_pvt_data_to_buffer(printAxis, -accelDistance_mm,    -print_speed_mm_per_s, accelTimeCnts);         // accelerate
        s << CMD::add_pvt_data_to_buffer(printAxis, -(lineLength_mm/2.0), -print_speed_mm_per_s, halfLinePrintTimeCnts); // constant velocity to trigger point
        s << CMD::add_pvt_data_to_buffer(printAxis, -(lineLength_mm/2.0), -print_speed_mm_per_s, halfLinePrintTimeCnts); // constant velocity
        s << CMD::add_pvt_data_to_buffer(printAxis, -accelDistance_mm,     0,                    accelTimeCnts);         // decelerate
        s << CMD::exit_pvt_mode(printAxis);
        s << CMD::set_reference_time();

        double linePrintTime_ms = linePrintTime_s * 1000.0;
        double halfLinePrintTime_ms = linePrintTime_ms / 2.0;
        double accelTime_ms = accelTime * 1000.0;
        if (triggerOffset_ms < halfLinePrintTime_ms) // trigger occurs during line print
        {
            qDebug() << "trigger occured during line print";

            int time1 = std::round(accelTime_ms);
            int time2 = std::round(accelTime_ms + halfLinePrintTime_ms - triggerOffset_ms);
            int time3 = std::round(accelTime_ms + linePrintTime_ms);

            s << CMD::begin_pvt_motion(printAxis);
            s << CMD::at_time_milliseconds(time1);
            s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
            if (time2 != time1) s << CMD::at_time_milliseconds(time2);
            s << CMD::set_bit(HS_TTL_BIT);
            s << CMD::at_time_milliseconds(time3);
        }
        else if (triggerOffset_ms < (halfLinePrintTime_ms + accelTime_ms)) // trigger occurs during acceleration
        {
            qDebug() << "trigger occured during acceleration";

            int time1 = std::round(accelTime_ms + halfLinePrintTime_ms - triggerOffset_ms);
            int time2 = std::round(accelTime_ms);
            int time3 = std::round(accelTime_ms + linePrintTime_ms);

            s << CMD::begin_pvt_motion(printAxis);
            if (time1 != 0) s << CMD::at_time_milliseconds(time1);
            s << CMD::set_bit(HS_TTL_BIT);
            if (time2 != time1) s << CMD::at_time_milliseconds(time2);
            s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
            s << CMD::at_time_milliseconds(time3);
        }
        else // trigger occurs before acceleration
        {
            qDebug() << "trigger occured before acceleration";

            int timeWaitBeforePrint = triggerOffset_ms - accelTime_ms - halfLinePrintTime_ms;
            int time1 = std::round(timeWaitBeforePrint);
            int time2 = std::round(timeWaitBeforePrint + accelTime_ms);
            int time3 = std::round(timeWaitBeforePrint + accelTime_ms + linePrintTime_ms);

            s << CMD::set_bit(HS_TTL_BIT);
            if (time1 != 0) s << CMD::at_time_milliseconds(time1);
            s << CMD::begin_pvt_motion(printAxis);
            if (time2 != time1) s << CMD::at_time_milliseconds(time2);
            s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(printAxis, dropletSpacing_um);
            s << CMD::at_time_milliseconds(time3);
        }

        s << CMD::disable_gearing_for(Axis::Jet);
        s << CMD::after_motion(printAxis);
        s << CMD::clear_bit(HS_TTL_BIT);
    }
    else // if printing with the x-axis, dont move anything for the flat, just trigger
    {
        s << CMD::set_bit(HS_TTL_BIT);
        s << CMD::at_time_milliseconds(100);
        s << CMD::clear_bit(HS_TTL_BIT);
    }

    s << CMD::set_jog(Axis::Jet, 1000); // jet at 1000z while waiting
    s << CMD::begin_motion(Axis::Jet);

    std::string returnString = CMD::cmd_buf_to_dmc(s);

    return returnString;
}
