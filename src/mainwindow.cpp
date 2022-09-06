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
#include "printerwidget.h"
#include "lineprintwidget.h"
#include "outputwindow.h"
#include "powdersetupwidget.h"

#include "jettingwidget.h"
#include "highspeedlinewidget.h"
#include "dropletobservationwidget.h"

#include <thread>

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mJetDrive = new JetDrive();

    // set up widgets
    mLinePrintingWidget       = new LinePrintWidget();
    mPowderSetupWidget        = new PowderSetupWidget();
    mHighSpeedLineWidget      = new HighSpeedLineWidget();
    mDropletObservationWidget = new DropletObservationWidget(mJetDrive);

    // add widgets to tabs on the top bar
    ui->tabWidget->addTab(mPowderSetupWidget, "Powder Setup");
    ui->tabWidget->addTab(mLinePrintingWidget, "Line Printing");
    ui->tabWidget->addTab(mHighSpeedLineWidget, "High-Speed Line Printing");
    ui->tabWidget->addTab(mDropletObservationWidget, "Jetting");

    // add dock widget for showing console output
    mDockWidget = new QDockWidget("Output Window",this);
    this->addDockWidget(Qt::RightDockWidgetArea, mDockWidget);
    mOutputWindow = new OutputWindow(this);
    mDockWidget->setWidget(mOutputWindow);
    mOutputWindow->print_string("Starting Program...");
    mOutputWindow->print_string("Program Started");

    // disable all buttons that require a controller connection
    allow_user_input(false);
}

// runs from main.cpp right after the object is initialized
void MainWindow::setup(Printer *printerPtr, PrintThread *printerThread)
{
    // assign pointers to member variables
    mPrinter     = printerPtr;
    mPrintThread = printerThread;

    // connect the string output from the printer thread to the output window widget
    connect(mPrintThread, &PrintThread::response, mOutputWindow, &OutputWindow::print_string);
    connect(mPrintThread, &PrintThread::ended, this, &MainWindow::thread_ended);
    connect(mPrintThread, &PrintThread::connected_to_controller, this, &MainWindow::connected_to_motion_controller);

    // connect signals for each of the printer widgets
    QList<PrinterWidget *> printerWidgets = this->findChildren<PrinterWidget *> ();
    for(int i{0}; i < printerWidgets.count(); ++i)
    {
        // qDebug() << "signals from " << printerWidgets[i]->accessibleName() << " connected";

        // share pointers with widgets
        printerWidgets[i]->pass_printer_objects(mPrinter, mPrintThread);

        // connect signals and slots
        connect(printerWidgets[i], &PrinterWidget::execute_command, mPrintThread, &PrintThread::execute_command); // connect "execute_command" signal on powder window to execute on thread
        connect(printerWidgets[i], &PrinterWidget::generate_printing_message_box, this, &MainWindow::generate_printing_message_box);
        connect(printerWidgets[i], &PrinterWidget::disable_user_input, this, [this]() {this->allow_user_input(false);});
        connect(printerWidgets[i], &PrinterWidget::print_to_output_window, this, &MainWindow::print_to_output_window);
        connect(printerWidgets[i], &PrinterWidget::stop_print_and_thread, this, &MainWindow::stop_print_and_thread);

        connect(printerWidgets[i], &PrinterWidget::jet_turned_on, mDropletObservationWidget, &DropletObservationWidget::jetting_was_turned_on);
        connect(printerWidgets[i], &PrinterWidget::jet_turned_off, mDropletObservationWidget, &DropletObservationWidget::jetting_was_turned_off);
    }

    // connect jog buttons
    connect(ui->xPositive, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->xNegative, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->yPositive, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->yNegative, &QAbstractButton::released, this, &MainWindow::jog_released);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::tab_was_changed);

    connect(ui->getXAxisPosition, &QAbstractButton::clicked, this, &MainWindow::get_current_x_axis_position);
    connect(ui->getYAxisPosition, &QAbstractButton::clicked, this, &MainWindow::get_current_y_axis_position);
    connect(ui->getZAxisPosition, &QAbstractButton::clicked, this, &MainWindow::get_current_z_axis_position);

    connect(ui->stopMotionButton, &QAbstractButton::clicked, this, &MainWindow::stop_button_pressed);

    connect(ui->zAbsoluteMoveButton, &QAbstractButton::clicked, this, &MainWindow::move_z_to_absolute_position);

}

// on application close
MainWindow::~MainWindow()
{
    delete ui;
    delete mJetDrive;
    if (mPrinter->g) // if there is an active connection to a controller
    {
        GCmd(mPrinter->g, "ST");
        GCmd(mPrinter->g, "MO"); // Turn off the motors
        GCmd(mPrinter->g, "CB 18"); // stop roller 1
        GCmd(mPrinter->g, "CB 21"); // stop roller 2
        GCmd(mPrinter->g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
        GClose(mPrinter->g); // Close the connection to the controller
    }
}

void MainWindow::on_connect_clicked()
{
    if (mPrinter->g == 0) // if there is no connection to the motion controller
    {
        std::stringstream s;

        s << CMD::open_connection_to_controller();
        s << CMD::set_default_controller_settings();
        s << CMD::homing_sequence();

        allow_user_input(false);
        mPrintThread->execute_command(s);

        // start connection to JetDrive on another thread
        std::thread jetDriveThread{&JetDrive::initialize_jet_drive, mJetDrive};
        jetDriveThread.detach();
    }
    else // if there is already a connection
    {
        allow_user_input(false);
        if (mPrinter->g)
        {
            GCmd(mPrinter->g, "ST");       // stop motion
            GCmd(mPrinter->g, "MO");       // disable Motors
            GClose(mPrinter->g);           // close connection to the motion controller
        }
        mPrinter->g = 0;               // Reset connection handle

        ui->connect->setText("Connect to Controller"); // change button label text
    }
}

void MainWindow::allow_user_input(bool allowed)
{
    //ui->activateHopper->setEnabled(allowed); // DISABLED FOR NOW
    ui->activateRoller1->setEnabled(allowed);
    //ui->activateRoller2->setEnabled(allowed);
    //ui->activateJet->setEnabled(allowed);
    ui->xHome->setEnabled(allowed);
    ui->yHome->setEnabled(allowed);
    ui->xPositive->setEnabled(allowed);
    ui->yPositive->setEnabled(allowed);
    ui->xNegative->setEnabled(allowed);
    ui->yNegative->setEnabled(allowed);
    ui->zDown->setEnabled(allowed);
    ui->zUp->setEnabled(allowed);
    ui->zMax->setEnabled(allowed);
    ui->zMin->setEnabled(allowed);
    ui->removeBuildBox->setEnabled(allowed);

    ui->getXAxisPosition->setEnabled(allowed);
    ui->getYAxisPosition->setEnabled(allowed);
    ui->getZAxisPosition->setEnabled(allowed);
    ui->zAbsoluteMoveButton->setEnabled(allowed);

    // set if user can input on all printer widgets
    QList<PrinterWidget *> printerWidgets = this->findChildren<PrinterWidget *> ();
    for(int i{0}; i < printerWidgets.count(); ++i)
    {
        printerWidgets[i]->allow_widget_input(allowed);
    }
}

void MainWindow::on_yPositive_pressed() // This name is a bit misleading (I need better +/- naming conventions)
{    
    std::stringstream s;

    s << CMD::set_accleration(Axis::Y, 300);
    s << CMD::set_deceleration(Axis::Y, 300);
    s << CMD::set_jog(Axis::Y, -ui->yVelocity->value());
    s << CMD::begin_motion(Axis::Y);

    mPrintThread->execute_command(s);
}

void MainWindow::on_xPositive_pressed()
{
    std::stringstream s;
    Axis x {Axis::X};

    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::set_jog(x, ui->xVelocity->value());
    s << CMD::begin_motion(x);

    mPrintThread->execute_command(s);
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

    mPrintThread->execute_command(s);
}

void MainWindow::on_yNegative_pressed()
{
    std::stringstream s;
    Axis y {Axis::Y};

    s << CMD::set_accleration(y, 300);
    s << CMD::set_deceleration(y, 300);
    s << CMD::set_jog(y, ui->yVelocity->value());
    s << CMD::begin_motion(y);

    // send off to thread to send to printer
    mPrintThread->execute_command(s);
}

void MainWindow::on_xNegative_pressed()
{
    std::stringstream s;
    Axis x {Axis::X};

    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::set_jog(x, -ui->xVelocity->value());
    s << CMD::begin_motion(x);

    mPrintThread->execute_command(s);
}

void MainWindow::on_xHome_clicked()
{
    std::stringstream s;

    Axis x{Axis::X};
    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::position_absolute(x, 0);
    s << CMD::set_speed(x, ui->xVelocity->value());
    s << CMD::begin_motion(x);
    s << CMD::motion_complete(x);

    allow_user_input(false);
    mPrintThread->execute_command(s);
}

void MainWindow::on_yHome_clicked()
{
    std::stringstream s;
    Axis y {Axis::Y};

    s << CMD::set_accleration(y, 200);
    s << CMD::set_deceleration(y, 200);
    s << CMD::position_absolute(y, 0);
    s << CMD::set_speed(y, ui->yVelocity->value());
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    allow_user_input(false);
    mPrintThread->execute_command(s);
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
    mPrintThread->execute_command(s);
}

void  MainWindow::on_zUp_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};

    s << CMD::position_relative(z, ui->zStepSize->value() / 1000.0); // convert from microns
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);


    allow_user_input(false);
    mPrintThread->execute_command(s);
}

void  MainWindow::on_zDown_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};

    s << CMD::position_relative(z, -ui->zStepSize->value() / 1000.0); // convert from microns
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    allow_user_input(false);
    mPrintThread->execute_command(s);
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
    mPrintThread->execute_command(s);
}

void MainWindow::on_activateHopper_stateChanged(int arg1)
{
    std::stringstream s;

    if (arg1 == 2) s << CMD::enable_hopper(); // box is checked
    else           s << CMD::disable_hopper();

    mPrintThread->execute_command(s);
}

void MainWindow::on_activateRoller1_toggled(bool checked)
{
    std::stringstream s;

    if (checked == 1) s << CMD::enable_roller1();
    else              s << CMD::disable_roller1();

    mPrintThread->execute_command(s);
}

void MainWindow::on_activateRoller2_toggled(bool checked)
{
    std::stringstream s;

    if (checked == 1) s << CMD::enable_roller2();
    else              s << CMD::disable_roller2();

    mPrintThread->execute_command(s);
}


void MainWindow::on_activateJet_stateChanged(int arg1)
{
    std::stringstream s;

    if (arg1 == 2)
    {        
        s << CMD::servo_here(Axis::Jet);
        s << CMD::set_accleration(Axis::Jet, 20000000); // set acceleration really high
        // TODO: get rid of magic numbers here, make a place for system defaults
        s << CMD::set_jog(Axis::Jet, 1024);             // set to jet at 1024hz
        // ADD OTHER JETTING SETTINGS HERE
        s << CMD::begin_motion(Axis::Jet);
    }
    else
    {
        s << CMD::stop_motion(Axis::Jet);
    }

    mPrintThread->execute_command(s);
}

void MainWindow::print_to_output_window(QString s)
{
    mOutputWindow->print_string(s);
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
    mPrintThread->execute_command(s);
}


void MainWindow::on_actionShow_Hide_Console_triggered()
{
    if (mDockWidget->isVisible()) mDockWidget->hide();
    else                          mDockWidget->show();
}

void MainWindow::generate_printing_message_box(const std::string &message)
{
    // This could be a way to do this in the future...
    //QProgressDialog *progressDialog = new QProgressDialog(this);
    //progressDialog->setLabelText("testing");
    //progressDialog->open();

    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(message));
    msgBox.setInformativeText("Click cancel to stop");
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    switch (ret)
    {
    case QMessageBox::Cancel: stop_print_and_thread(); break;
    default: break;
    }
}

void MainWindow::stop_button_pressed()
{
    if (mPrinter->g)
    {
        stop_print_and_thread();
        mDropletObservationWidget->jetting_was_turned_off();
    }
}

void MainWindow::stop_print_and_thread()
{
    mPrintThread->stop();
    if (mPrinter->g)
    {
        // Can't use CMD:: commands here...
        // work on a way to either send these through the thread even though it is blocked
        // or be able to strip CMD:: commands of new line and beginning command type so I can use them here
        GCmd(mPrinter->g, "HX");    // Halt any running program
        GCmd(mPrinter->g, "ST");    // stop motion on all axes
        GCmd(mPrinter->g, "CB 18"); // stop roller 1
        GCmd(mPrinter->g, "CB 21"); // stop roller 2
        GCmd(mPrinter->g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
    }
}

void MainWindow::get_current_x_axis_position()
{
    if (mPrinter->g)
    {
        int currentXPos;
        GCmdI(mPrinter->g, "TPX", &currentXPos);
        double currentXPos_mm = currentXPos / (double)X_CNTS_PER_MM;
        print_to_output_window("Current X: " + QString::number(currentXPos_mm) + "mm");
    }
}

void MainWindow::get_current_y_axis_position()
{
    if (mPrinter->g)
    {
        int currentYPos;
        GCmdI(mPrinter->g, "TPY", &currentYPos);
        double currentYPos_mm = currentYPos / (double)Y_CNTS_PER_MM;
        print_to_output_window("Current Y: " + QString::number(currentYPos_mm) + "mm");
    }
}

void MainWindow::get_current_z_axis_position()
{
    if (mPrinter->g)
    {
        int currentZPos;
        GCmdI(mPrinter->g, "TPZ", &currentZPos);
        double currentZPos_mm = currentZPos / (double)Z_CNTS_PER_MM;
        print_to_output_window("Current Z: " + QString::number(currentZPos_mm) + "mm");
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
    mPrintThread->execute_command(s);
}

void MainWindow::thread_ended()
{
    allow_user_input(true);
}

void MainWindow::resizeEvent(QResizeEvent* event) // override the resize event of the main window
{
   QMainWindow::resizeEvent(event);
   // Your code here.
   mHighSpeedLineWidget->reset_preview_zoom();
}

void MainWindow::tab_was_changed(int index) // code that gets run when the current tab in the mainwindow is changed
{
    if (index == ui->tabWidget->indexOf(mHighSpeedLineWidget))
    {
        mHighSpeedLineWidget->reset_preview_zoom();
    }
}

void MainWindow::connected_to_motion_controller()
{
    ui->connect->setText("Disconnect Controller");
}
