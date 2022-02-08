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
#include <QDebug>

#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"
#include "JetServer.h"
#include "printer.h"
#include "printhread.h"
#include "printerwidget.h"
#include "lineprintwidget.h"
#include "outputwindow.h"
#include "powdersetupwidget.h"

#include "jettingwidget.h"
#include "highspeedlinewidget.h"
#include "dropletobservationwidget.h"

inline void split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

MainWindow::MainWindow(QMainWindow *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // revert values to defaults from file (this isn't working right now)
    on_revertDefault_clicked();

    // set up widgets
    mLinePrintingWidget = new LinePrintWidget();
    mPowderSetupWidget = new PowderSetupWidget();
    mJettingWidget = new JettingWidget();
    mHighSpeedLineWidget = new HighSpeedLineWidget();
    mDropletObservationWidget = new DropletObservationWidget();
    ui->tabWidget->addTab(mPowderSetupWidget, "Powder Setup");
    ui->tabWidget->addTab(mLinePrintingWidget, "Line Printing");
    ui->tabWidget->addTab(mJettingWidget, "Jetting");
    ui->tabWidget->addTab(mHighSpeedLineWidget, "High-Speed Line Printing");
    ui->tabWidget->addTab(mDropletObservationWidget, "Droplet Observation");

    mDockWidget = new QDockWidget("Output Window",this);
    this->addDockWidget(Qt::RightDockWidgetArea, mDockWidget);
    mOutputWindow = new OutputWindow(this);
    mDockWidget->setWidget(mOutputWindow);

    mOutputWindow->print_string("Starting Program...");
    mOutputWindow->print_string("Program Started");

    //Disable all buttons that require a controller connection
    allow_user_input(false);
}

void MainWindow::setup(Printer *printerPtr, PrintThread *printerThread)
{
    mPrinter = printerPtr;
    mPrintThread = printerThread;

    // Connect the string output from the printer thread to the output window widget
    connect(mPrintThread, &PrintThread::response, mOutputWindow, &OutputWindow::print_string);
    connect(mPrintThread, &PrintThread::ended, this, &MainWindow::thread_ended);
    connect(mPrintThread, &PrintThread::connected_to_controller, this, &MainWindow::connected_to_motion_controller);

    // connect signals for each of the printer widgets
    QList<PrinterWidget *> printerWidgets = this->findChildren<PrinterWidget *> ();
    for(int i{0}; i < printerWidgets.count(); ++i)
    {
        qDebug() << "signals from " << printerWidgets[i]->accessibleName() << " connected";

       connect(printerWidgets[i], &PrinterWidget::execute_command, mPrintThread, &PrintThread::execute_command); // connect "execute_command" signal on powder window to execute on thread
       connect(printerWidgets[i], &PrinterWidget::generate_printing_message_box, this, &MainWindow::generate_printing_message_box);
    }

    // connect jog buttons
    connect(ui->xPositive, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->xNegative, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->yPositive, &QAbstractButton::released, this, &MainWindow::jog_released);
    connect(ui->yNegative, &QAbstractButton::released, this, &MainWindow::jog_released);

    connect(mJettingWidget, &JettingWidget::start_jetting, this, &MainWindow::start_jetting);
    connect(mJettingWidget, &JettingWidget::stop_jetting, this, &MainWindow::stop_jetting);

}

void MainWindow::thread_ended()
{
    allow_user_input(true);
}

MainWindow::~MainWindow()
{
    // On application close
    delete ui;
    if(mPrinter->g)
    { // if there is an active connection to a controller
        GCmd(mPrinter->g, "MO"); // Turn off the motors
        GClose(mPrinter->g);
    } // Close the connection to the controller
}

void MainWindow::connected_to_motion_controller()
{
    ui->connect->setText("Disconnect Controller");
}

void MainWindow::allow_user_input(bool allowed)
{
    ui->activateHopper->setEnabled(allowed);
    ui->activateRoller1->setEnabled(allowed);
    ui->activateRoller2->setEnabled(allowed);
    ui->activateJet->setEnabled(allowed);
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
    Axis x{Axis::X};

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
    Axis y{Axis::Y};

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
    Axis x{Axis::X};

    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::set_jog(x, -ui->xVelocity->value());
    s << CMD::begin_motion(x);

    mPrintThread->execute_command(s);
}

void MainWindow::on_xHome_clicked()
{
    // LOOK INTO USING THE "HM" COMMAND INSTEAD OF "FI"?
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
    Axis y{Axis::Y};

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
    Axis z{Axis::Z};

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
    Axis z{Axis::Z};

    s << CMD::position_relative(z, ui->zStepSize->value() / 1000.0); // convert to microns
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
    Axis z{Axis::Z};

    s << CMD::position_relative(z, -ui->zStepSize->value() / 1000.0); // convert to microns
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
    Axis z{Axis::Z};

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

    if(arg1 == 2)
    {
        s << CMD::enable_hopper();
    }
    else
    {
        s << CMD::disable_hopper();
    }

    mPrintThread->execute_command(s);
}

void MainWindow::on_connect_clicked()
{
    if(mPrinter->g == 0)
    {
        std::stringstream s;

        s << "GOpen"                     << "\n";   // Establish connection with motion controller

        // Controller Configuration
        s << CMD::detail::GCmd() << "MO"              << "\n";   // Ensure motors are off for setup

        // X Axis
        s << CMD::detail::GCmd() << "MTX=-1"          << "\n";   // Set motor type to reversed brushless
        s << CMD::detail::GCmd() << "CEX=2"           << "\n";   // Set Encoder to reversed quadrature
        s << CMD::detail::GCmd() << "BMX=40000"       << "\n";   // Set magnetic pitch of linear motor
        s << CMD::detail::GCmd() << "AGX=1"           << "\n";   // Set amplifier gain
        s << CMD::detail::GCmd() << "AUX=9"           << "\n";   // Set current loop (based on inductance of motor)
        s << CMD::detail::GCmd() << "TLX=3"           << "\n";   // Set constant torque limit to 3V
        s << CMD::detail::GCmd() << "TKX=0"           << "\n";   // Disable peak torque setting for now

        // Set PID Settings
        s << CMD::detail::GCmd() << "KDX=250"         << "\n";   // Set Derivative
        s << CMD::detail::GCmd() << "KPX=40"          << "\n";   // Set Proportional
        s << CMD::detail::GCmd() << "KIX=2"           << "\n";   // Set Integral
        s << CMD::detail::GCmd() << "PLX=0.1"         << "\n";   // Set low-pass filter

        // Y Axis
        s << CMD::detail::GCmd() << "MTY=1"           << "\n";   // Set motor type to standard brushless
        s << CMD::detail::GCmd() << "CEY=0"           << "\n";   // Set Encoder to reversed quadrature??? (or is it?)
        s << CMD::detail::GCmd() << "BMY=2000"        << "\n";   // Set magnetic pitch of rotary motor
        s << CMD::detail::GCmd() << "AGY=1"           << "\n";   // Set amplifier gain
        s << CMD::detail::GCmd() << "AUY=11"          << "\n";   // Set current loop (based on inductance of motor)
        s << CMD::detail::GCmd() << "TLY=6"           << "\n";   // Set constant torque limit to 6V
        s << CMD::detail::GCmd() << "TKY=0"           << "\n";   // Disable peak torque setting for now
        // Set PID Settings
        s << CMD::detail::GCmd() << "KDY=500"         << "\n";   // Set Derivative
        s << CMD::detail::GCmd() << "KPY=70"          << "\n";   // Set Proportional
        s << CMD::detail::GCmd() << "KIY=1.7002"      << "\n";   // Set Integral

        // Z Axis
        s << CMD::detail::GCmd() << "MTZ=-2.5"        << "\n";   // Set motor type to standard brushless
        s << CMD::detail::GCmd() << "CEZ=14"          << "\n";   // Set Encoder to reversed quadrature
        s << CMD::detail::GCmd() << "AGZ=0"           << "\n";   // Set amplifier gain
        s << CMD::detail::GCmd() << "AUZ=9"           << "\n";   // Set current loop (based on inductance of motor)
        // Note: There might be more settings especially for this axis I might want to add later

        // H Axis (Jetting Axis)
        s << CMD::detail::GCmd() << "MTH=-2"          << "\n";   // Set jetting axis to be stepper motor with defualt low
        s << CMD::detail::GCmd() << "AGH=0"           << "\n";   // Set gain to lowest value
        s << CMD::detail::GCmd() << "LDH=3"           << "\n";   // Disable limit sensors for H axis
        s << CMD::detail::GCmd() << "KSH=0.25"        << "\n";   // Minimize filters on step signals
        s << CMD::detail::GCmd() << "ITH=1"           << "\n";   // Minimize filters on step signals

        s << CMD::detail::GCmd() << "CC 19200,0,1,0"  << "\n";   //AUX PORT FOR THE ULTRASONIC GENERATOR
        s << CMD::detail::GCmd() << "CN=-1"           << "\n";   // Set correct polarity for all limit switches
        s << CMD::detail::GCmd() << "BN"              << "\n";   // Save (burn) these settings to the controller just to be safe
        s << CMD::detail::GCmd() << "SH XYZ"          << "\n";   // Enable X,Y, and Z motors


        // === Home the X-Axis using the central home sensor index pulse ===

        s << CMD::set_accleration(Axis::X, 800);
        s << CMD::set_deceleration(Axis::X, 800);
        s << CMD::set_limit_switch_deceleration(Axis::X, 800);
        s << CMD::set_jog(Axis::X, -25); // jog towards rear limit

        s << CMD::set_accleration(Axis::Y, 400);
        s << CMD::set_deceleration(Axis::Y, 400);
        s << CMD::set_limit_switch_deceleration(Axis::Y, 600);
        s << CMD::set_jog(Axis::Y, 25); // jog towards front limit

        s << CMD::set_accleration(Axis::Z, 20);
        s << CMD::set_deceleration(Axis::Z, 20);
        s << CMD::set_limit_switch_deceleration(Axis::Z, 40);
        s << CMD::set_jog(Axis::Z, -2);                       // jog to bottom (MAX SPEED of 5mm/s!)
        s << CMD::disable_forward_software_limit(Axis::Z);    // turn off top software limit

        s << CMD::begin_motion(Axis::X);
        s << CMD::begin_motion(Axis::Y);
        s << CMD::begin_motion(Axis::Z);

        s << CMD::motion_complete(Axis::X);
        s << CMD::motion_complete(Axis::Y);
        s << CMD::motion_complete(Axis::Z);

        s << CMD::sleep(1000);

        s << CMD::set_jog(Axis::X, 30);
        s << CMD::set_homing_velocity(Axis::X, 0.5);
        s << CMD::find_index(Axis::X);

        s << CMD::set_speed(Axis::Y, 40);
        s << CMD::position_relative(Axis::Y, -200);

        s << CMD::set_accleration(Axis::Z, 10);        // slower acceleration for going back up
        s << CMD::set_speed(Axis::Z, 2);
        s << CMD::position_relative(Axis::Z, 13.5322); // TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!

        s << CMD::begin_motion(Axis::X);
        s << CMD::begin_motion(Axis::Y);
        s << CMD::begin_motion(Axis::Z);

        s << CMD::motion_complete(Axis::X);
        s << CMD::motion_complete(Axis::Y);
        s << CMD::motion_complete(Axis::Z);

        s << CMD::set_speed(Axis::X, 50);
        s << CMD::position_relative(Axis::X, -40);
        s << CMD::begin_motion(Axis::X);
        s << CMD::motion_complete(Axis::X);

        s << CMD::define_position(Axis::X, X_STAGE_LEN_MM / 2.0);
        s << CMD::define_position(Axis::Y, 0);
        s << CMD::define_position(Axis::Z, 0);
        s << CMD::set_forward_software_limit(Axis::Z, 0); // set software limit to current position

        allow_user_input(false);
        mPrintThread->execute_command(s);
        jetter_setup();
    }
    else
    {
        GCmd(mPrinter->g, "MO");       // Disable Motors
        GClose(mPrinter->g);           // close connection to the motion controller
        mPrinter->g = 0;               // Reset connection handle

        ui->connect->setText("Connect to Controller");

        allow_user_input(false);
    }
}

void MainWindow::on_saveDefault_clicked()
{
    std::ofstream ofs;
    ofs.open("C:/Users/ME/Documents/GitHub/CREATE_LAB_Binder_Jet_Printer/PrinterSettings.txt", std::ofstream::out | std::ofstream::trunc);
    ofs << "XAxisVelocity\t" << std::to_string(ui->xVelocity->value())  << "\n";
    ofs << "YAxisVelocity\t" << std::to_string(ui->yVelocity->value())  << "\n";
    ofs << "ZStepSize\t" << std::to_string(ui->zStepSize->value())      << "\n";
    ofs.close();
}

void MainWindow::on_revertDefault_clicked()
{
    std::string line;
    std::ifstream myfile("C:/Users/ME/Documents/GitHub/CREATE_LAB_Binder_Jet_Printer/PrinterSettings.txt");
    if (myfile.is_open())
    {
        while(getline(myfile, line)) {
            std::vector<std::string> row_values;

            split(line, '\t', row_values);

            if(row_values[0] == "XAxisVelocity")
            {
                ui->xVelocity->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "YAxisVelocity")
            {
                ui->yVelocity->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "ZStepSize")
            {
                ui->zStepSize->setValue(stoi(row_values[1]));
            }
        }
        myfile.close();
    }
    else std::cout << "Unable to open settings file\n";//Notify user-> "Unable to open file";
}


void MainWindow::on_activateRoller1_toggled(bool checked)
{
    std::stringstream s;

    if(checked == 1)
    {
        s << CMD::enable_roller1();
    }
    else
    {
        s << CMD::disable_roller1();
    }

    mPrintThread->execute_command(s);
}

void MainWindow::on_activateRoller2_toggled(bool checked)
{
    std::stringstream s;

    if(checked == 1)
    {
        s << CMD::enable_roller2();
    }
    else
    {
        s << CMD::disable_roller2();
    }

    mPrintThread->execute_command(s);
}


void MainWindow::on_activateJet_stateChanged(int arg1)
{
    std::stringstream s;

    if(arg1 == 2)
    {        
        s << CMD::servo_here(Axis::Jet);
        s << CMD::set_accleration(Axis::Jet, 20000000); // set acceleration really high
        s << CMD::set_jog(Axis::Jet, 1000);             // set to jet at 1000hz
        // ADD OTHER JETTING SETTINGS HERE
        s << CMD::begin_motion(Axis::Jet);
    }
    else
    {
        s << CMD::stop_motion(Axis::Jet);
    }

    mPrintThread->execute_command(s);
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
    if(mDockWidget->isVisible())
    {
        mDockWidget->hide();
    }else
    {
        mDockWidget->show();
    }
}

void MainWindow::generate_printing_message_box(const std::string &message)
{
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(message));
    msgBox.setInformativeText("Click cancel to stop");
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Cancel:
        mPrintThread->stop();
        if(mPrinter->g)
        {
            // Can't use CMD:: commands here...
            // work on a way to either send these through the thread even though it is blocked
            // or be able to strip CMD:: commands of new line and beginning command type so I can use them here
            GCmd(mPrinter->g, "ST"); // stop motion
            GCmd(mPrinter->g, "CB 18"); // stop roller 1
            GCmd(mPrinter->g, "CB 21"); // stop roller 2
            GCmd(mPrinter->g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
        }
        break;
    default:
        break;
    }
}

void MainWindow::start_jetting()
{
    int port{9};
    gjets1.fFrequency = 1000L;
    SendCommand(port, MFJDRV_FREQUENCY, 0.1);

    gjets1.fMode = 1;
    SendCommand(port, MFJDRV_CONTMODE, 0.1);

    gjets1.fSource = 0;               // set internal trigger
    SendCommand(port, MFJDRV_SOURCE, 0.1); // set trigger source

    // Start Jetting
    SendCommand(port, MFJDRV_SOFTTRIGGER, .1); // This command turns on continuous jetting if the trigger source is set to internal
}

void MainWindow::stop_jetting()
{
    int port{9};
    gjets1.fMode = 0;
    SendCommand(port, MFJDRV_CONTMODE, .1); // This command sets trigger mode to single (also, turns off continuous jetting from the soft trigger)

    gjets1.fSource = 1;               // set external trigger
    SendCommand(port, MFJDRV_SOURCE, 0.1); // set trigger source
}
