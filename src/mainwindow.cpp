#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>
#include <thread>
#include <QDockWidget>
#include <thread>

#include <QMessageBox>
#include <QProgressDialog>
#include <QDebug>

#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

#include "jetdrive.h"
#include "printer.h"
#include "printhread.h"
#include "mjdriver.h"

#include "printerwidget.h"
#include "lineprintwidget.h"
#include "outputwindow.h"
#include "powdersetupwidget.h"
#include "highspeedlinewidget.h"
#include "dropletobservationwidget.h"
#include "bedmicroscopewidget.h"
#include "mjprintheadwidget.h"

#include "pcd.h"
#include "dmc4080.h"
#include "mister.h"

MainWindow::MainWindow(Printer *printer_, QMainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    printer(printer_)
{
    ui->setupUi(this);
    ui->homeZAxisCheckBox->setChecked(false); // Set Z-home to off by default as currently it rams the z-axis into the print heads.

    // set up widgets (parent is set when adding as a tab)
    linePrintingWidget       = new LinePrintWidget(printer);
    powderSetupWidget        = new PowderSetupWidget(printer);
    highSpeedLineWidget      = new HighSpeedLineWidget(printer);
    dropletObservationWidget = new DropletObservationWidget(printer);
    bedMicroscopeWidget      = new BedMicroscopeWidget(printer);
    mjPrintheadWidget        = new MJPrintheadWidget(printer);

    // add widgets to tabs on the top bar (tab widget now owns)
    ui->tabWidget->addTab(powderSetupWidget, "Powder Setup");
    ui->tabWidget->addTab(linePrintingWidget, "Line Printing");
    ui->tabWidget->addTab(highSpeedLineWidget, "High-Speed Line Printing");
    ui->tabWidget->addTab(dropletObservationWidget, "Jetting");
    ui->tabWidget->addTab(bedMicroscopeWidget, "Bed Imaging");
    ui->tabWidget->addTab(mjPrintheadWidget, "MJ Printhead");

    // set fill background for all tab widgets
    for (int i{0}; i < ui->tabWidget->count(); ++i)
    {
        auto widget = ui->tabWidget->widget(i);
        widget->setAutoFillBackground(true);
    }

    // open a txt file for logging commands
    open_log_file();
    logFile << "Application opened at "
            << QDateTime::currentDateTime()
               .toString("yyyy.MM.dd hh:mm")
               .toStdString()
            << "\n";

    // add dock widget for showing console output
    dockWidget = new QDockWidget("Output Window", this);
    this->addDockWidget(Qt::RightDockWidgetArea, dockWidget);
    outputWindow = new OutputWindow(this, logFile);
    dockWidget->setWidget(outputWindow);
    outputWindow->print_string("Starting Program...");
    outputWindow->print_string("Program Started");

    // disable all buttons that require a controller connection
    allow_user_input(false);

    connect(printer->mcu->messagePoller, &GMessagePoller::message, this, &MainWindow::print_to_output_window);

    // print message box
    messageBox = new QMessageBox(this);
    messageBox->setInformativeText("Click cancel to stop");
    messageBox->setStandardButtons(QMessageBox::Cancel);
    messageBox->setDefaultButton(QMessageBox::Cancel);

    connect(printer->mcu->printerThread, &PrintThread::ended, messageBox, &QMessageBox::close);
    connect(messageBox, &QMessageBox::rejected, this, &MainWindow::stop_print_and_thread);

    messageHandler = new GMessageHandler(printer, this);
    connect(printer->mcu->messagePoller, &GMessagePoller::message, messageHandler, &GMessageHandler::handle_message);

    // export image when printer requests
    connect(messageHandler, &GMessageHandler::capture_microscope_image, bedMicroscopeWidget, &BedMicroscopeWidget::export_image);

}

// on application close
MainWindow::~MainWindow()
{
    printer->disconnect_printer();

    delete ui;

    // log application close and close the file
    logFile << "Application closed at "
            << QDateTime::currentDateTime()
               .toString("yyyy.MM.dd hh:mm")
               .toStdString()
            << "\n";
    logFile.close();
}

// runs from main.cpp right after the object is initialized
void MainWindow::setup()
{

    // connect the output from the printer thread to the output window widget
    connect(printer->mcu->printerThread, &PrintThread::response, outputWindow, &OutputWindow::print_string);
    connect(printer->mcu->printerThread, &PrintThread::ended, this, &MainWindow::thread_ended);
    connect(printer->mcu->printerThread, &PrintThread::connected_to_controller, this, &MainWindow::connected_to_motion_controller);

    // connect signals for each of the printer widgets (this works recursively)
    QList<PrinterWidget*> printerWidgets = this->findChildren<PrinterWidget*>();
    for(int i{0}; i < printerWidgets.count(); ++i)
    {
        // connect signals and slots
        connect(printerWidgets[i], &PrinterWidget::execute_command, printer->mcu->printerThread, &PrintThread::execute_command);
        connect(printerWidgets[i], &PrinterWidget::generate_printing_message_box, this, &MainWindow::generate_printing_message_box);
        connect(printerWidgets[i], &PrinterWidget::disable_user_input, this, [this]() {this->allow_user_input(false);});
        connect(printerWidgets[i], &PrinterWidget::print_to_output_window, this, &MainWindow::print_to_output_window);
        connect(printerWidgets[i], &PrinterWidget::stop_print_and_thread, this, &MainWindow::stop_print_and_thread);

        connect(printerWidgets[i], &PrinterWidget::start_continuous_jetting, dropletObservationWidget, &DropletObservationWidget::start_jetting);
        connect(printerWidgets[i], &PrinterWidget::stop_continuous_jetting, dropletObservationWidget, &DropletObservationWidget::stop_jetting);
    }

    // connect jog buttons
    connect(ui->xRightButton, &QAbstractButton::pressed, this, &MainWindow::x_right_button_pressed);
    connect(ui->xLeftButton, &QAbstractButton::pressed, this, &MainWindow::x_left_button_pressed);
    connect(ui->yUpButton, &QAbstractButton::pressed, this, &MainWindow::y_up_button_pressed);
    connect(ui->yDownButton, &QAbstractButton::pressed, this, &MainWindow::y_down_button_pressed);

    connect(ui->xRightButton, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->xLeftButton, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->yUpButton, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->yDownButton, &QAbstractButton::released, this, &MainWindow::jog_released);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::tab_was_changed);

    connect(ui->getXAxisPosition, &QAbstractButton::clicked, this, &MainWindow::get_current_x_axis_position);
    connect(ui->getYAxisPosition, &QAbstractButton::clicked, this, &MainWindow::get_current_y_axis_position);
    connect(ui->getZAxisPosition, &QAbstractButton::clicked, this, &MainWindow::get_current_z_axis_position);

    connect(ui->moveTopRight, &QPushButton::clicked, this, &MainWindow::moveTopRight_clicked);
    connect(ui->moveTopLeft, &QPushButton::clicked, this, &MainWindow::moveTopLeft_clicked);
    connect(ui->moveBotRight, &QPushButton::clicked, this, &MainWindow::moveBotRight_clicked);
    connect(ui->moveBotLeft, &QPushButton::clicked, this, &MainWindow::moveBotLeft_clicked);
    connect(ui->moveCenter, &QPushButton::clicked, this, &MainWindow::moveCenter_clicked);

    connect(ui->moveToX, &QPushButton::clicked, this, &MainWindow::moveToX_clicked);
    connect(ui->moveToY, &QPushButton::clicked, this, &MainWindow::moveToY_clicked);



    connect(ui->stopMotionButton, &QAbstractButton::clicked,
            this, [this]()
    {
        if (printer->mcu->g)
        {
            stop_print_and_thread();
            dropletObservationWidget->stop_jetting();
        }

    });

    connect(ui->zAbsoluteMoveButton, &QAbstractButton::clicked, this, &MainWindow::move_z_to_absolute_position);



    connect(ui->actionShow_Hide_Droplet_Tool, &QAction::triggered, this, &MainWindow::show_hide_droplet_analyzer_window);

    // connect response from jetDrive to output window
    connect(printer->jetDrive, &JetDrive::Controller::response, this, &MainWindow::print_to_output_window);
    connect(printer->jetDrive, &JetDrive::Controller::timeout, this, &MainWindow::print_to_output_window);
    connect(printer->jetDrive, &JetDrive::Controller::error, this, &MainWindow::print_to_output_window);

    connect(ui->connectToJetDriveButton, &QPushButton::clicked, this, &MainWindow::connect_to_jet_drive_button_pressed);
    connect(ui->connecttoPCDButton, &QPushButton::clicked, this, &MainWindow::connect_to_pressure_controller_button_pressed);
    connect(ui->connectToMCUButton, &QPushButton::clicked, this, &MainWindow::connect_motion_controller_button_pressed);
    connect(ui->connectToMisterButton, &QPushButton::clicked, this, [=]{printer->mister->connect_to_misters();});

    // connect response from pressure controller to output window
    // TODO: Do I want these here?
    connect(printer->pressureController, &PCD::Controller::response, this, &MainWindow::print_to_output_window);
    connect(printer->pressureController, &PCD::Controller::timeout, this, &MainWindow::print_to_output_window);
    connect(printer->pressureController, &PCD::Controller::error, this, &MainWindow::print_to_output_window);

    connect(printer->mister, &Mister::Controller::response, this, &MainWindow::print_to_output_window);
    connect(printer->mister, &Mister::Controller::timeout, this, &MainWindow::print_to_output_window);
    connect(printer->mister, &Mister::Controller::error, this, &MainWindow::print_to_output_window);

    connect(printer->mjController, &Added_Scientific::Controller::response, this, &MainWindow::print_to_output_window);
    connect(printer->mjController, &Added_Scientific::Controller::timeout, this, &MainWindow::print_to_output_window);
    connect(printer->mjController, &Added_Scientific::Controller::error, this, &MainWindow::print_to_output_window);

    // Connect the new Y-Axis Init button (added 12/11)
    connect(ui->initYAxisButton, &QPushButton::clicked, this, &MainWindow::initialize_y_axis_commutation);
}

void MainWindow::on_connect_clicked()
{
    if (printer->mcu->g == 0) // if there is no connection to the motion controller
    {
        //get connection settings / options from UI
        bool homeZAxis = ui->homeZAxisCheckBox->isChecked();
        allow_user_input(false);
        printer->connect(homeZAxis);
    }
    else // if there is already a connection
    {
        allow_user_input(false);
        printer->disconnect_printer();

        // change button label text
        ui->connect->setText("\nConnect and Home Printer\n");
        ui->homeZAxisCheckBox->setEnabled(true);

    }
}

void MainWindow::allow_user_input(bool allowed)
{
    ui->activateRoller1->setEnabled(allowed);
    ui->xHome->setEnabled(allowed);
    ui->yHome->setEnabled(allowed);
    ui->xRightButton->setEnabled(allowed);
    ui->yUpButton->setEnabled(allowed);
    ui->xLeftButton->setEnabled(allowed);
    ui->yDownButton->setEnabled(allowed);
    ui->zDown->setEnabled(allowed);
    ui->zUp->setEnabled(allowed);
    ui->zMax->setEnabled(allowed);
    ui->zMin->setEnabled(allowed);
    ui->removeBuildBox->setEnabled(allowed);

    ui->getXAxisPosition->setEnabled(allowed);
    ui->getYAxisPosition->setEnabled(allowed);
    ui->getZAxisPosition->setEnabled(allowed);
    ui->zAbsoluteMoveButton->setEnabled(allowed);

    ui->moveBotLeft->setEnabled(allowed);
    ui->moveBotRight->setEnabled(allowed);
    ui->moveTopLeft->setEnabled(allowed);
    ui->moveTopRight->setEnabled(allowed);
    ui->moveCenter->setEnabled(allowed);


    // set if user can input on all printer widgets
    QList<PrinterWidget*> printerWidgets = this->findChildren<PrinterWidget*>();
    for(int i{0}; i < printerWidgets.count(); ++i)
    {
        printerWidgets[i]->allow_widget_input(allowed);
    }
}

void MainWindow::y_up_button_pressed()
{    
    std::stringstream s;

    s << CMD::set_accleration(Axis::Y, 300);
    s << CMD::set_deceleration(Axis::Y, 300);
    s << CMD::set_jog(Axis::Y, -ui->yVelocity->value());
    s << CMD::begin_motion(Axis::Y);

    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::x_right_button_pressed()
{
    std::stringstream s;
    Axis x {Axis::X};

    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::set_jog(x, ui->xVelocity->value());
    s << CMD::begin_motion(x);

    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::jog_released()
{
    std::stringstream s;
    s << CMD::stop_motion(Axis::X);
    s << CMD::stop_motion(Axis::Y);
    s << CMD::stop_motion(Axis::Z);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);

    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::y_down_button_pressed()
{
    std::stringstream s;
    Axis y {Axis::Y};

    s << CMD::set_accleration(y, 300);
    s << CMD::set_deceleration(y, 300);
    s << CMD::set_jog(y, ui->yVelocity->value());
    s << CMD::begin_motion(y);

    // send off to thread to send to printer
    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::x_left_button_pressed()
{
    std::stringstream s;
    Axis x {Axis::X};

    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::set_jog(x, -ui->xVelocity->value());
    s << CMD::begin_motion(x);

    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::on_xHome_clicked()
{
    std::stringstream s;

    Axis x{Axis::X};
    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::position_absolute(x, 0);
    s << CMD::set_speed(x, 50);
    s << CMD::begin_motion(x);
    s << CMD::motion_complete(x);

    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::on_yHome_clicked()
{
    std::stringstream s;
    Axis y {Axis::Y};

    s << CMD::set_accleration(y, 300);
    s << CMD::set_deceleration(y, 300);
    s << CMD::position_absolute(y, 0);
    s << CMD::set_speed(y, 50);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::on_zMax_clicked()
{     
    std::stringstream s;
    Axis z {Axis::Z};

    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_jog(z, 1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::on_zUp_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};

    // 1. Get the desired movement distance from the UI
    double stepValue_microns = ui->zStepSize->value();
    double stepValue_mm = stepValue_microns / 1000.0 * (1.0 + (ui->scaleValue->value() / 100.0));

    // 2. Add the relative move command to the buffer (positive value for moving up)
    s << CMD::position_relative(z, stepValue_mm);
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    // 3. Get the starting position BEFORE the move
    int startPos_counts;
    GCmdI(printer->mcu->g, "TPZ", &startPos_counts); // Gets position in raw counts

    // 4. Calculate the start and predicted end positions in mm
    double startPos_mm = startPos_counts / (double)Z_CNTS_PER_MM;
    double predictedEndPos_mm = startPos_mm + stepValue_mm; // Add because we are moving up

    // 5. Create a clear display message
    std::string message = "Z: " + std::to_string(startPos_mm) + " -> " + std::to_string(predictedEndPos_mm) + " mm (+" + std::to_string(stepValue_mm) + "mm)";
    s << CMD::display_message(message);

    // 6. Execute all buffered commands
    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::on_zDown_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};

    // 1. Get the desired movement distance from the UI
    double stepValue_microns = ui->zStepSize->value();
    double stepValue_mm = stepValue_microns / 1000.0 * (1.0 + (ui->scaleValue->value() / 100.0));

    // 2. Add the relative move command to the buffer
    //    (Note the negative sign for moving down)
    s << CMD::position_relative(z, -stepValue_mm);
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    // 3. Get the starting position BEFORE the move
    int startPos_counts;
    GCmdI(printer->mcu->g, "TPZ", &startPos_counts); // Gets position in raw counts

    // 4. Calculate the start and predicted end positions in mm
    double startPos_mm = startPos_counts / (double)Z_CNTS_PER_MM;
    double predictedEndPos_mm = startPos_mm - stepValue_mm; // Subtract because we are moving down

    // 5. Create a clear display message
    std::string message = "Z: " + std::to_string(startPos_mm) + " -> " + std::to_string(predictedEndPos_mm) + " mm (-" + std::to_string(stepValue_mm) + "mm)";
    s << CMD::display_message(message);

    // 6. Execute all buffered commands
    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}

void  MainWindow::on_zMin_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};

    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_jog(z, -1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::on_activateRoller1_toggled(bool checked)
{
    std::stringstream s;

    if (checked == 1) s << CMD::enable_roller1();
    else              s << CMD::disable_roller1();

    printer->mcu->printerThread->execute_command(s);
}

// NEW MOVE COMMANDS
void MainWindow::moveTopRight_clicked(){
    double xLocation = 88;
    double yLocation = -28;

    std::stringstream s_cmdMove;

    // --- 1. **Build commands for Y-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::Y, 800);
    s_cmdMove << CMD::set_deceleration(Axis::Y, 800);
    s_cmdMove << CMD::set_speed(Axis::Y, 60);
    s_cmdMove << CMD::position_absolute(Axis::Y, yLocation);
    s_cmdMove << CMD::begin_motion(Axis::Y);
    s_cmdMove << CMD::after_motion(Axis::Y);

    // --- 2. **Build commands for X-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::X, 600);
    s_cmdMove << CMD::set_deceleration(Axis::X, 600);
    s_cmdMove << CMD::set_speed(Axis::X, 60);
    s_cmdMove << CMD::position_absolute(Axis::X, xLocation);
    s_cmdMove << CMD::begin_motion(Axis::X);
    s_cmdMove << CMD::after_motion(Axis::X);

    // --- 3. **Add completion message and compile program** ---
    s_cmdMove << CMD::display_message("Arrived at Location \"Top Right\"");

    printer->mcu->printerThread->execute_command(s_cmdMove);
}

void MainWindow::moveTopLeft_clicked(){
    double xLocation = 5.5;
    double yLocation = -28;

    std::stringstream s_cmdMove;

    // --- 1. **Build commands for Y-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::Y, 800);
    s_cmdMove << CMD::set_deceleration(Axis::Y, 800);
    s_cmdMove << CMD::set_speed(Axis::Y, 60);
    s_cmdMove << CMD::position_absolute(Axis::Y, yLocation);
    s_cmdMove << CMD::begin_motion(Axis::Y);
    s_cmdMove << CMD::after_motion(Axis::Y);

    // --- 2. **Build commands for X-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::X, 600);
    s_cmdMove << CMD::set_deceleration(Axis::X, 600);
    s_cmdMove << CMD::set_speed(Axis::X, 60);
    s_cmdMove << CMD::position_absolute(Axis::X, xLocation);
    s_cmdMove << CMD::begin_motion(Axis::X);
    s_cmdMove << CMD::after_motion(Axis::X);

    // --- 3. **Add completion message and compile program** ---
    s_cmdMove << CMD::display_message("Arrived at Location \"Top Left\"");

    printer->mcu->printerThread->execute_command(s_cmdMove);
}

void MainWindow::moveBotRight_clicked(){
    double xLocation = 88;
    double yLocation = -118;

    std::stringstream s_cmdMove;

    // --- 1. **Build commands for Y-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::Y, 800);
    s_cmdMove << CMD::set_deceleration(Axis::Y, 800);
    s_cmdMove << CMD::set_speed(Axis::Y, 60);
    s_cmdMove << CMD::position_absolute(Axis::Y, yLocation);
    s_cmdMove << CMD::begin_motion(Axis::Y);
    s_cmdMove << CMD::after_motion(Axis::Y);

    // --- 2. **Build commands for X-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::X, 600);
    s_cmdMove << CMD::set_deceleration(Axis::X, 600);
    s_cmdMove << CMD::set_speed(Axis::X, 60);
    s_cmdMove << CMD::position_absolute(Axis::X, xLocation);
    s_cmdMove << CMD::begin_motion(Axis::X);
    s_cmdMove << CMD::after_motion(Axis::X);

    // --- 3. **Add completion message and compile program** ---
    s_cmdMove << CMD::display_message("Arrived at Location \"Bottom Right\"");

    printer->mcu->printerThread->execute_command(s_cmdMove);
}

void MainWindow::moveBotLeft_clicked(){
    double xLocation = 5.5;
    double yLocation = -118;

    std::stringstream s_cmdMove;

    // --- 1. **Build commands for Y-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::Y, 800);
    s_cmdMove << CMD::set_deceleration(Axis::Y, 800);
    s_cmdMove << CMD::set_speed(Axis::Y, 60);
    s_cmdMove << CMD::position_absolute(Axis::Y, yLocation);
    s_cmdMove << CMD::begin_motion(Axis::Y);
    s_cmdMove << CMD::after_motion(Axis::Y);

    // --- 2. **Build commands for X-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::X, 600);
    s_cmdMove << CMD::set_deceleration(Axis::X, 600);
    s_cmdMove << CMD::set_speed(Axis::X, 60);
    s_cmdMove << CMD::position_absolute(Axis::X, xLocation);
    s_cmdMove << CMD::begin_motion(Axis::X);
    s_cmdMove << CMD::after_motion(Axis::X);

    // --- 3. **Add completion message and compile program** ---
    s_cmdMove << CMD::display_message("Arrived at Location \"Bottom Left\"");

    printer->mcu->printerThread->execute_command(s_cmdMove);
}

void MainWindow::moveCenter_clicked(){
    double xLocation = 50;
    double yLocation = -73;

    std::stringstream s_cmdMove;

    // --- 1. **Build commands for Y-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::Y, 800);
    s_cmdMove << CMD::set_deceleration(Axis::Y, 800);
    s_cmdMove << CMD::set_speed(Axis::Y, 60);
    s_cmdMove << CMD::position_absolute(Axis::Y, yLocation);
    s_cmdMove << CMD::begin_motion(Axis::Y);
    s_cmdMove << CMD::after_motion(Axis::Y);

    // --- 2. **Build commands for X-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::X, 600);
    s_cmdMove << CMD::set_deceleration(Axis::X, 600);
    s_cmdMove << CMD::set_speed(Axis::X, 60);
    s_cmdMove << CMD::position_absolute(Axis::X, xLocation);
    s_cmdMove << CMD::begin_motion(Axis::X);
    s_cmdMove << CMD::after_motion(Axis::X);

    // --- 3. **Add completion message and compile program** ---
    s_cmdMove << CMD::display_message("Arrived at Location \"Center\"");

    printer->mcu->printerThread->execute_command(s_cmdMove);
    allow_user_input(false);
}

void MainWindow::moveToX_clicked(){

    // OFFSET VALUES HARDCODED
    double Printer2NozzleOffsetX{-12.48};
    double Printer2NozzleOffsetY{-182.484};

    double xLocation = ui->xMoveLoc->value() + Printer2NozzleOffsetX;
    //double yLocation = ui->yMoveLoc->value();

    std::stringstream s_cmdMoveX;

    // --- 1. **Build commands for X-axis movement** ---
    s_cmdMoveX << CMD::set_accleration(Axis::X, 600);
    s_cmdMoveX << CMD::set_deceleration(Axis::X, 600);
    s_cmdMoveX << CMD::set_speed(Axis::X, 60);
    s_cmdMoveX << CMD::position_absolute(Axis::X, xLocation);
    s_cmdMoveX << CMD::begin_motion(Axis::X);
    s_cmdMoveX << CMD::after_motion(Axis::X);

    // --- 2. **Add completion message and compile program** ---
    CMD::string moveMSG = "Moved to: " + std::to_string(xLocation);
    s_cmdMoveX << CMD::display_message(moveMSG);

    // --- 3. **Execute the compiled program** ---
    printer->mcu->printerThread->execute_command(s_cmdMoveX);
}

void MainWindow::moveToY_clicked(){

    // OFFSET VALUES HARDCODED
    double Printer2NozzleOffsetX{-12.48};
    double Printer2NozzleOffsetY{-182.484};

    //double xLocation = ui->xMoveLoc->value();
    double yLocation = ui->yMoveLoc->value() + Printer2NozzleOffsetY;

    std::stringstream s_cmdMoveY;

    // --- 1. **Build commands for Y-axis movement** ---
    s_cmdMoveY << CMD::set_accleration(Axis::Y, 600);
    s_cmdMoveY << CMD::set_deceleration(Axis::Y, 600);
    s_cmdMoveY << CMD::set_speed(Axis::X, 60);
    s_cmdMoveY << CMD::position_absolute(Axis::Y, yLocation);
    s_cmdMoveY << CMD::begin_motion(Axis::Y);
    s_cmdMoveY << CMD::after_motion(Axis::Y);

    // --- 2. **Add completion message and compile program** ---
    CMD::string moveMSG = "Moved to: " + std::to_string(yLocation);
    s_cmdMoveY << CMD::display_message(moveMSG);

    // --- 3. **Execute the compiled program** ---
    printer->mcu->printerThread->execute_command(s_cmdMoveY);
}


void MainWindow::print_to_output_window(QString s)
{
    outputWindow->print_string(s);
}

void MainWindow::on_removeBuildBox_clicked()
{
    std::stringstream s;

    int yAxisAcceleration{50};
    int yAxisJogVelocity{30};
    int zAxisAcceleration{20};
    int zAxisJogVelocity{-2};
    int sleepTimeMilliseconds{500};
    double zAxisOffsetFromLowerLimit{2};
    int zAxisOffsetMoveSpeed{1};

    s << CMD::set_accleration(Axis::Y, yAxisAcceleration);
    s << CMD::set_deceleration(Axis::Y, yAxisAcceleration);
    s << CMD::set_jog(Axis::Y, yAxisJogVelocity); // jog to front

    s << CMD::set_accleration(Axis::Z, zAxisAcceleration);
    s << CMD::set_deceleration(Axis::Z, zAxisAcceleration);
    s << CMD::set_jog(Axis::Z, zAxisJogVelocity); // jog to bottom

    s << CMD::begin_motion(Axis::Y);
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);

    s << CMD::sleep(sleepTimeMilliseconds);
    s << CMD::position_relative(Axis::Z, zAxisOffsetFromLowerLimit);
    s << CMD::set_speed(Axis::Z, zAxisOffsetMoveSpeed);

    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);

    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}


void MainWindow::on_actionShow_Hide_Console_triggered()
{
    if (dockWidget->isVisible()) dockWidget->hide();
    else                         dockWidget->show();
}

void MainWindow::show_hide_droplet_analyzer_window()
{
    if (!dropletObservationWidget->is_droplet_anlyzer_window_visible())
        dropletObservationWidget->show_droplet_analyzer_widget();
    else
        dropletObservationWidget->hide_droplet_analyzer_widget();
}

void MainWindow::generate_printing_message_box(const std::string &message)
{
    messageBox->setText(QString::fromStdString(message));
    messageBox->open();
}

void MainWindow::stop_print_and_thread()
{
    printer->mcu->printerThread->stop();
    if (printer->mcu->g)
    {
        // TODO: Can't use CMD:: commands here... Fix this when CMD::
        //       commands are changed to just be dmc commands
        // work on a way to either send these through the thread
        // even though it is blocked or be able to strip CMD:: commands
        // of new line and beginning command type so I can use them here
        GCmd(printer->mcu->g, "HX");    // Halt any running program
        GCmd(printer->mcu->g, "ST");    // stop motion on all axes
        GCmd(printer->mcu->g, "CB 18"); // stop roller 1
        GCmd(printer->mcu->g, "CB 21"); // stop roller 2
        GCmd(printer->mcu->g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
    }
}

void MainWindow::get_current_x_axis_position()
{
    if (printer->mcu->g)
    {
        int currentXPos;
        GCmdI(printer->mcu->g, "TPX", &currentXPos);
        double currentXPos_mm = currentXPos / (double)X_CNTS_PER_MM;
        print_to_output_window("Current X: "
                               + QString::number(currentXPos_mm)
                               + "mm");
    }
}

void MainWindow::get_current_y_axis_position()
{
    if (printer->mcu->g)
    {
        int currentYPos;
        GCmdI(printer->mcu->g, "TPY", &currentYPos);
        double currentYPos_mm = currentYPos / (double)Y_CNTS_PER_MM;
        print_to_output_window("Current Y: "
                               + QString::number(currentYPos_mm)
                               + "mm");
    }
}

void MainWindow::get_current_z_axis_position()
{
    if (printer->mcu->g)
    {
        int currentZPos;
        GCmdI(printer->mcu->g, "TPZ", &currentZPos);
        double currentZPos_mm = currentZPos / (double)Z_CNTS_PER_MM;
        print_to_output_window("Current Z: "
                               + QString::number(currentZPos_mm)
                               + "mm");
    }
}

void MainWindow::move_z_to_absolute_position()
{
    std::stringstream s;

    s << CMD::position_absolute(Axis::Z, ui->zAbsoluteMoveSpinBox->value());
    s << CMD::set_accleration(Axis::Z, 10);
    s << CMD::set_deceleration(Axis::Z, 10);
    s << CMD::set_speed(Axis::Z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);

    allow_user_input(false);
    printer->mcu->printerThread->execute_command(s);
}

void MainWindow::open_log_file()
{
    QString folderName = "BJ_Logs";
    QString logDir =
            QStandardPaths::writableLocation
            (QStandardPaths::DocumentsLocation)
            + "/"
            + folderName;
    if(!QDir(logDir).exists()) QDir().mkdir(logDir);
    QDateTime date = QDateTime::currentDateTime();
    QString fileName = logDir
            + "/BJ_Log_"
            + date.toString("yyyy_MM_dd")
            + ".txt";
    logFile.open(fileName.toStdString(), std::ios::app); // append
}

void MainWindow::connect_to_jet_drive_button_pressed()
{
    printer->jetDrive->connect_to_jet_drive();
}

void MainWindow::connect_to_pressure_controller_button_pressed()
{
    printer->pressureController->connect_to_pressure_controller();
}

void MainWindow::connect_to_mister_button_pressed()
{
    printer->mister->connect_to_misters();
}

void MainWindow::connect_motion_controller_button_pressed()
{
    // TODO: make this use connect functions from mcu
    if (printer->mcu->g == 0) // if there is no connection to the motion controller
    {
        std::stringstream s;

        s << CMD::open_connection_to_controller();
        s << CMD::set_default_controller_settings();
        printer->mcu->printerThread->execute_command(s);

        // subscribe to messages
        printer->mcu->messagePoller->connect_to_controller(printer->mcu->address);
    }
}

void MainWindow::thread_ended()
{
    allow_user_input(true);
}

// override the resize event of the main window
void MainWindow::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   highSpeedLineWidget->reset_preview_zoom();
}

// code that gets run when the current tab in the mainwindow is changed
void MainWindow::tab_was_changed(int index)
{
    if (index == ui->tabWidget->indexOf(highSpeedLineWidget))
    {
        highSpeedLineWidget->reset_preview_zoom();
    }
}

void MainWindow::connected_to_motion_controller()
{
    ui->connect->setText("\nDisconnect Controller\n");
    ui->homeZAxisCheckBox->setEnabled(false);
}


// Galil script that calibrates the y-axis when it is broken
void MainWindow::initialize_y_axis_commutation()
{

    std::string program =
        "REM Y-Axis Commutation Initialization\r"
        "MOY\r"               // Motor Off Y
        "BAY\r"               // Brushless Axis Y
        "BMY=2000\r"          // Brushless Modulo Y
        "BIY=-1\r"            // Brushless Init Y
        "BCY\r"               // Brushless Calibration Y
        "hall=_QHY\r"         // Store Hall State
        "SHY\r"               // Servo Here Y
        "JGY=-1600\r"         // Jog Y Negative (-1600)
        "BGY\r"               // Begin Jog Y
        "#hall\r"             // Label for loop
        "WT2\r"               // Wait 2ms
        "JP#hall,_QHY=hall\r" // Jump back if hall state is same
        "STY\r"               // Stop Y
        "AMY\r"               // After Motion (Wait for stop)
        "MG \"Y-Axis Initialization Complete\"\r" // Prints to Output Window
        "EN\r";               // End Program

    if (printer->mcu->g)
    {
        // Print "Starting" message
        outputWindow->print_string("Starting Y-Axis Initialization (Negative Dir)...");

        // 1. Download the program to the controller
        GProgramDownload(printer->mcu->g, program.c_str(), "");

        // 2. Execute the program
        std::stringstream s;
        s << "GCmd," << "XQ" << "\n";
        printer->mcu->printerThread->execute_command(s);
    }
    else
    {
        outputWindow->print_string("Error: Controller not connected.");
    }
}


#include "moc_mainwindow.cpp"
