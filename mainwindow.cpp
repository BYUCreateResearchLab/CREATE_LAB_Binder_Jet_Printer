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

#include "JetServer.h"
#include "printer.h"
#include "printhread.h"
#include "commandcodes.h"

void split(const std::string &s, char delim, std::vector<std::string> &elems)
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
    //Set up Second Window
    sWindow = new progWindow();
    sWindow->setWindowState(Qt::WindowMaximized); // set line printing window to be maximized
    connect(sWindow, &progWindow::firstWindow, this, &MainWindow::show);

    //Initialize Necessary Variables
    mDeltaX = 10;
    mDeltaY = 10;
    mDeltaZ = 15;
    MainWindow::on_revertDefault_clicked();


    //Disable all buttons that require a controller connection
    allow_user_input(true);


    mDockWidget = new QDockWidget("Output Window",this);
    this->addDockWidget(Qt::RightDockWidgetArea, mDockWidget);
    mOutputWindow = new OutputWindow(this);
    mDockWidget->setWidget(mOutputWindow);
    mOutputWindow->print_string("Starting Program...");
}

void MainWindow::setup(Printer *printerPtr, PrintThread *printerThread)
{
    printer = printerPtr;
    mPrinterThread = printerThread;
    sWindow->setup(printerPtr, printerThread, mOutputWindow);

    // Connect the string output from the printer thread to the output window widget
    connect(mPrinterThread, &PrintThread::response, mOutputWindow, &OutputWindow::print_string);
    connect(mPrinterThread, &PrintThread::ended, this, &MainWindow::thread_ended);
    connect(sWindow, &progWindow::printing_from_prog_window, this, &MainWindow::disable_user_input);
    connect(mPrinterThread, &PrintThread::connected_to_controller, this, &MainWindow::connected_to_motion_controller);
}

void MainWindow::thread_ended()
{
    allow_user_input(true);
}

MainWindow::~MainWindow()
{
    // On application close
    delete ui;
    if(printer->g)
    { // if there is an active connection to a controller
        GCmd(printer->g, "MO"); // Turn off the motors
        GClose(printer->g);
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
    ui->zHome->setEnabled(allowed);
    ui->xPositive->setEnabled(allowed);
    ui->yPositive->setEnabled(allowed);
    ui->xNegative->setEnabled(allowed);
    ui->yNegative->setEnabled(allowed);
    ui->zDown->setEnabled(allowed);
    ui->zUp->setEnabled(allowed);
    ui->zMax->setEnabled(allowed);
    ui->zMin->setEnabled(allowed);
    ui->spreadNewLayer->setEnabled(allowed);

    sWindow->set_connected(allowed); // Disable print buttons in line printing window
}

void MainWindow::disable_user_input()
{
    allow_user_input(false);
}

void MainWindow::on_yPositive_clicked() // This name is a bit misleading (I need better +/- naming conventions)
{    
    std::stringstream s;

    s << GCmd() << "ACY="  << mm2cnts(100, 'Y')                     << "\n";
    s << GCmd() << "DCY="  << mm2cnts(100, 'Y')                     << "\n";
    s << GCmd() << "SPY="  << mm2cnts(ui->yVelocity->value(), 'Y')  << "\n";
    s << GCmd() << "PRY="  << mm2cnts(-ui->yDistance->value(), 'Y') << "\n";
    s << GCmd() << "BGY"                                            << "\n";
    s << GMotionComplete() << "Y"                                   << "\n";

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_xPositive_clicked()
{
    std::stringstream s;

    s << GCmd() << "ACX=" << mm2cnts(800, 'X')                    << "\n";
    s << GCmd() << "DCX=" << mm2cnts(800, 'X')                    << "\n";
    s << GCmd() << "SPX=" << mm2cnts(ui->xVelocity->value(), 'X') << "\n";
    s << GCmd() << "PRX=" << mm2cnts(ui->xDistance->value(), 'X') << "\n";
    s << GCmd() << "BGX"                                          << "\n";
    s << GMotionComplete() << "X"                                 << "\n";

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_yNegative_clicked()
{
    std::stringstream s;

    // Gcommands
    // generating motion controller langauge to send to printer
    s << GCmd() << "ACY="  << mm2cnts(100, 'Y')                     << "\n"; // set y-axis acceleration
    s << GCmd() << "DCY="  << mm2cnts(100, 'Y')                     << "\n";
    s << GCmd() << "SPY="  << mm2cnts(ui->yVelocity->value(), 'Y')  << "\n";
    s << GCmd() << "PRY="  << mm2cnts(ui->yDistance->value(), 'Y')  << "\n";
    s << GCmd() << "BGY"                                            << "\n";
    s << GMotionComplete() << "Y"                                   << "\n";

    // send off to thread to send to printer
    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_xNegative_clicked()
{
    std::stringstream s;

    s << GCmd() << "ACX=" << mm2cnts(800, 'X')                    << "\n";
    s << GCmd() << "DCX=" << mm2cnts(800, 'X')                    << "\n";
    s << GCmd() << "SPX=" << mm2cnts(ui->xVelocity->value(), 'X') << "\n";
    s << GCmd() << "PRX=" << mm2cnts(-ui->xDistance->value(), 'X')<< "\n";
    s << GCmd() << "BGX"                                          << "\n";
    s << GMotionComplete() << "X"                                 << "\n";

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_xHome_clicked()
{
    std::stringstream s;

    // take commands that I do use and put them in documentation for people to know
    // where to look for appropriate GCode commands

    s << GCmd() << "ACX="  << mm2cnts(800, 'X')                  << "\n";  // x-axis accleration
    s << GCmd() << "DCX="  << mm2cnts(800, 'X')                  << "\n";  // x-axis deceleration
    s << GCmd() << "JGX="  << mm2cnts(-15, 'X')                  << "\n";  // x-axis jog velocity
    s << GCmd() << "BGX"                                         << "\n";  // begin motion in x-axis
    s << GMotionComplete() << "X"                                << "\n";  // block until complete
    s << GCmd() << "JGX="  << mm2cnts(15, 'X')                   << "\n";  // x-axis jog velocity
    s << GCmd() << "HVX="  << mm2cnts(0.5, 'X')                  << "\n";  // 0.5 mm/s on second move towards home sensor
    s << GCmd() << "FIX"                                         << "\n";  // Find index command for x axis
    s << GCmd() << "BGX"                                         << "\n";  // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
    s << GMotionComplete() << "X"                                << "\n";  // Wait until X stage finishes moving
    s << GCmd() << "DPX="  << mm2cnts(X_STAGE_LEN_MM / 2, 'X')   << "\n";  //Offset position so "0" is the rear limit (home is at center of stage)

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_yHome_clicked()
{
    std::stringstream s;

    s << GCmd() << "ACY="  << mm2cnts(200, 'Y')  << "\n"; // y-axis acceleration
    s << GCmd() << "DCY="  << mm2cnts(200, 'Y')  << "\n"; // deceleration
    s << GCmd() << "JGY="  << mm2cnts(30, 'Y')   << "\n"; // jog speed
    s << GCmd() << "BGY"                         << "\n"; // begin motion
    s << GMotionComplete() << "Y"                << "\n"; // wait until complete
    s << GCmd() << "SPY="  << mm2cnts(25, 'Y')   << "\n"; // print speed
    s << GCmd() << "PRY="  << mm2cnts(-200, 'Y') << "\n"; // position relative
    s << GCmd() << "BGY"                         << "\n"; // begin motion
    s << GMotionComplete() << "Y"                << "\n"; // wait until complete

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_zHome_clicked()
{
    std::stringstream s;

    s << GCmd() << "ACZ="  << mm2cnts(10, 'Z')   << "\n";
    s << GCmd() << "DCZ="  << mm2cnts(10, 'Z')   << "\n";
    s << GCmd() << "JGZ="  << mm2cnts(-1.5, 'Z') << "\n";
    s << GCmd() << "BGZ"                         << "\n";
    s << GMotionComplete() << "Z"                << "\n";

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_zStepSize_valueChanged(int arg1)
{
    mDeltaZ = arg1;
}

void MainWindow::on_zMax_clicked()
{     
    std::stringstream s;

    s << GCmd() << "ACZ="  << mm2cnts(10, 'Z')   << "\n"; // acceleration
    s << GCmd() << "DCZ="  << mm2cnts(10, 'Z')   << "\n"; // deceleration
    s << GCmd() << "JGZ="  << mm2cnts(1.5, 'Z')  << "\n"; // jog speed
    s << GCmd() << "BGZ"                         << "\n"; // begin motion
    s << GMotionComplete() << "Z"                << "\n"; // wait until complete

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void  MainWindow::on_zUp_clicked()
{
    std::stringstream s;

    s << GCmd() << "PRZ="  << um2cnts(ui->zStepSize->value(), 'Z') << "\n"; // position relative
    s << GCmd() << "ACZ="  << mm2cnts(10, 'Z')                     << "\n"; // acceleration
    s << GCmd() << "DCZ="  << mm2cnts(10, 'Z')                     << "\n"; // deceleration
    s << GCmd() << "SPZ="  << mm2cnts(1.5, 'Z')                    << "\n"; // speed
    s << GCmd() << "BGZ"                                           << "\n"; // begin motion
    s << GMotionComplete() << "Z"                                  << "\n"; // wait until complete

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void  MainWindow::on_zDown_clicked()
{
    std::stringstream s;

    s << GCmd() << "PRZ="  << um2cnts(-ui->zStepSize->value(), 'Z') << "\n"; // position relative
    s << GCmd() << "ACZ="  << mm2cnts(10, 'Z')                      << "\n"; // acceleration
    s << GCmd() << "DCZ="  << mm2cnts(10, 'Z')                      << "\n"; // deceleration
    s << GCmd() << "SPZ="  << mm2cnts(1.5, 'Z')                     << "\n"; // speed
    s << GCmd() << "BGZ"                                            << "\n"; // begin motion
    s << GMotionComplete() << "Z"                                   << "\n"; // wait until complete

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void  MainWindow::on_zMin_clicked()
{
    std::stringstream s;

    s << GCmd() << "ACZ="  << mm2cnts(10, 'Z')   << "\n"; // acceleration
    s << GCmd() << "DCZ="  << mm2cnts(10, 'Z')   << "\n"; // deceleration
    s << GCmd() << "JGZ="  << mm2cnts(-1.5, 'Z') << "\n"; // jog speed
    s << GCmd() << "BGZ"                         << "\n"; // begin motion
    s << GMotionComplete() << "Z"                << "\n"; // wait until complete

    allow_user_input(false);
    mPrinterThread->execute_command(s);
}

void MainWindow::on_activateHopper_stateChanged(int arg1)
{
    std::stringstream s;

    if(arg1 == 2)
    {
        //ACTIVATE ULTRASONIC GENERATOR
        s << GCmd() << "MG{P2} {^85}, {^49}, {^13}{N}" << "\n";
    }
    else
    {
        //DISACTIVATE ULTRASONIC GENERATOR
        s << GCmd() << "MG{P2} {^85}, {^48}, {^13}{N}" << "\n";
    }

    mPrinterThread->execute_command(s);
}

void MainWindow::on_connect_clicked()
{
    if(printer->g == 0)
    {
        //TO DO - DISABLE BUTTONS UNTIL TUNED, SEPERATE INITIALIZATION HOMING BUTTONS

        std::stringstream s;

        s << "GOpen"                     << "\n";   // Establish connection with motion controller

        //s << GCmd() << "SH XYZ"          << "\n";   // enable X, Y, and Z motors

        // Controller Configuration
        s << GCmd() << "MO"              << "\n";   // Ensure motors are off for setup

        // X Axis
        s << GCmd() << "MTX=-1"          << "\n";   // Set motor type to reversed brushless
        s << GCmd() << "CEX=2"           << "\n";   // Set Encoder to reversed quadrature
        s << GCmd() << "BMX=40000"       << "\n";   // Set magnetic pitch of linear motor
        s << GCmd() << "AGX=1"           << "\n";   // Set amplifier gain
        s << GCmd() << "AUX=9"           << "\n";   // Set current loop (based on inductance of motor)
        s << GCmd() << "TLX=3"           << "\n";   // Set constant torque limit to 3V
        s << GCmd() << "TKX=0"           << "\n";   // Disable peak torque setting for now

        // Set PID Settings
        s << GCmd() << "KDX=250"         << "\n";   // Set Derivative
        s << GCmd() << "KPX=40"          << "\n";   // Set Proportional
        s << GCmd() << "KIX=2"           << "\n";   // Set Integral
        s << GCmd() << "PLX=0.1"         << "\n";   // Set low-pass filter

        // Y Axis
        s << GCmd() << "MTY=1"           << "\n";   // Set motor type to standard brushless
        s << GCmd() << "CEY=0"           << "\n";   // Set Encoder to reversed quadrature??? (or is it?)
        s << GCmd() << "BMY=2000"        << "\n";   // Set magnetic pitch of rotary motor
        s << GCmd() << "AGY=1"           << "\n";   // Set amplifier gain
        s << GCmd() << "AUY=11"          << "\n";   // Set current loop (based on inductance of motor)
        s << GCmd() << "TLY=6"           << "\n";   // Set constant torque limit to 6V
        s << GCmd() << "TKY=0"           << "\n";   // Disable peak torque setting for now
        // Set PID Settings
        s << GCmd() << "KDY=500"         << "\n";   // Set Derivative
        s << GCmd() << "KPY=70"          << "\n";   // Set Proportional
        s << GCmd() << "KIY=1.7002"      << "\n";   // Set Integral

        // Z Axis
        s << GCmd() << "MTZ=-2.5"        << "\n";   // Set motor type to standard brushless
        s << GCmd() << "CEZ=14"          << "\n";   // Set Encoder to reversed quadrature
        s << GCmd() << "AGZ=0"           << "\n";   // Set amplifier gain
        s << GCmd() << "AUZ=9"           << "\n";   // Set current loop (based on inductance of motor)
        // Note: There might be more settings especially for this axis I might want to add later

        // H Axis (Jetting Axis)
        s << GCmd() << "MTH=-2"          << "\n";   // Set jetting axis to be stepper motor with defualt low
        s << GCmd() << "AGH=0"           << "\n";   // Set gain to lowest value
        s << GCmd() << "LDH=3"           << "\n";   // Disable limit sensors for H axis
        s << GCmd() << "KSH=0.25"        << "\n";   // Minimize filters on step signals
        s << GCmd() << "ITH=1"           << "\n";   // Minimize filters on step signals

        s << GCmd() << "CC 19200,0,1,0"  << "\n";   //AUX PORT FOR THE ULTRASONIC GENERATOR
        s << GCmd() << "CN=-1"           << "\n";   // Set correct polarity for all limit switches
        s << GCmd() << "BN"              << "\n";   // Save (burn) these settings to the controller just to be safe
        s << GCmd() << "SH XYZ"          << "\n";   // Enable X,Y, and Z motors


        // === Home the X-Axis using the central home sensor index pulse ===

        s << GCmd() << "ACX=" << mm2cnts(800, 'X')     << "\n";   //
        s << GCmd() << "DCX=" << mm2cnts(800, 'X')     << "\n";   //
        s << GCmd() << "SDX=" << mm2cnts(800, 'X')     << "\n";   //
        s << GCmd() << "JGX=" << mm2cnts(-15, 'X')     << "\n";   // jog towards rear limit

        s << GCmd() << "ACY=" << mm2cnts(200, 'Y')     << "\n";   //
        s << GCmd() << "DCY=" << mm2cnts(200, 'Y')     << "\n";   //
        s << GCmd() << "JGY=" << mm2cnts(20, 'Y')      << "\n";   // jog towards front limit

        s << GCmd() << "ACZ=" << mm2cnts(10, 'Z')      << "\n";   //Acceleration of C     757760 steps ~ 1 mm
        s << GCmd() << "DCZ=" << mm2cnts(10, 'Z')      << "\n";   //Deceleration of C     7578 steps ~ 1 micron
        s << GCmd() << "JGZ=" << mm2cnts(-1.5, 'Z')    << "\n";   // Speed of Z
        s << GCmd() << "FLZ=2147483647"                << "\n";   // Turn off forward software limit during homing

        s << GCmd() << "BGX"                           << "\n";
        s << GCmd() << "BGY"                           << "\n";
        s << GCmd() << "BGZ"                           << "\n";

        jetter_setup();

        s << GMotionComplete() << "X"                  << "\n";
        s << GMotionComplete() << "Y"                  << "\n";
        s << GMotionComplete() << "Z"                  << "\n";

        s << GCmd() << "JGX="  << mm2cnts(15, 'X')     << "\n"; // 15 mm/s jog towards home sensor
        s << GCmd() << "HVX="  << mm2cnts(0.5, 'X')    << "\n"; // 0.5 mm/s on second move towards home sensor
        s << GCmd() << "FIX"                           << "\n"; // Find index command for x axis

        s << GCmd() << "ACY=" << mm2cnts(100, 'Y')     << "\n"; // 50 mm/s^2
        s << GCmd() << "DCY=" << mm2cnts(100, 'Y')     << "\n"; // 50 mm/s^2
        s << GCmd() << "SPY=" << mm2cnts(25, 'Y')      << "\n"; // 25 mm/s
        s << GCmd() << "PRY=" << mm2cnts(-160, 'Y')    << "\n"; // move the y-axis for setting the 'home' position

        s << GCmd() << "ACZ=" << mm2cnts(10, 'Z')      << "\n";
        s << GCmd() << "DCZ=" << mm2cnts(10, 'Z')      << "\n";
        s << GCmd() << "SDZ=" << mm2cnts(20, 'Z')      << "\n"; // Sets deceleration when limit switch is touched
        s << GCmd() << "SPZ=" << mm2cnts(1.5, 'Z')     << "\n";
        s << GCmd() << "PRZ=" << mm2cnts(13.5322, 'Z') << "\n"; // TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!

        s << GCmd() << "BGX"                           << "\n"; // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
        s << GCmd() << "BGY"                           << "\n";
        s << GCmd() << "BGZ"                           << "\n";

        s << GMotionComplete() << "X"                  << "\n";
        s << GMotionComplete() << "Y"                  << "\n";
        s << GMotionComplete() << "Z"                  << "\n";

        s << GCmd() << "SPX=" << mm2cnts(30, 'X')      << "\n"; // set x-speed
        s << GCmd() << "PRX=" << mm2cnts(-40, 'X')     << "\n"; // OFFSET TO ACCOUNT FOR THE OFF-CENTER BINDER JET HEAD LOCATION
        s << GCmd() << "BGX"                           << "\n";
        s << GMotionComplete() << "X"                  << "\n"; // Wait until X stage finishes moving

        s << GCmd() << "DPX=75000"                     << "\n"; // Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
        s << GCmd() << "DPY=0"                         << "\n"; // sets the current position as 0 Do we need this?
        s << GCmd() << "DPZ=0"                         << "\n";
        s << GCmd() << "FLZ=0"                         << "\n"; // Set software limit on z so it can't go any higher than current position

        allow_user_input(false);
        mPrinterThread->execute_command(s);
    }
    else
    {
        GCmd(printer->g, "MO");       // Disable Motors
        GClose(printer->g);           // close connection to the motion controller
        printer->g = 0;               // Reset connection handle

        ui->connect->setText("Connect to Controller");

        allow_user_input(false);
    }
}

void MainWindow::on_OpenProgramWindow_clicked()
{
    sWindow->show();
    this->close();
}

void MainWindow::on_saveDefault_clicked()
{
    std::ofstream ofs;
    ofs.open("C:/Users/ME/Documents/GitHub/CREATE_LAB_Binder_Jet_Printer/PrinterSettings.txt", std::ofstream::out | std::ofstream::trunc);
    ofs << "XAxisVelocity\t" << std::to_string(ui->xVelocity->value()) << "\n";
    ofs << "YAxisVelocity\t" << std::to_string(ui->yVelocity->value()) << "\n";
    ofs << "XAxisDistance\t" << std::to_string(ui->xDistance->value()) << "\n";
    ofs << "YAxisDistance\t" << std::to_string(ui->yDistance->value()) << "\n";
    ofs << "ZStepSize\t" << std::to_string(ui->zStepSize->value()) << "\n";
    ofs << "RollerSpeed\t" << std::to_string(ui->rollerSpeed->value()) << "\n";
    ofs << "NumberOfLayers\t" << std::to_string(ui->numLayers->value()) << "\n";
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
            else if(row_values[0] == "XAxisDistance")
            {
                ui->xDistance->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "YAxisDistance")
            {
                ui->yDistance->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "ZStepSize")
            {
                ui->zStepSize->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "RollerSpeed")
            {
                ui->rollerSpeed->setValue(stoi(row_values[1]));
            }
            else if(row_values[0] == "NumberOfLayers")
            {
                ui->numLayers->setValue(stoi(row_values[1]));
            }
        }
        myfile.close();
    }
    else std::cout << "Unable to open file";//Notify user-> "Unable to open file";
}

void MainWindow::on_spreadNewLayer_clicked()
{
    for(int i = 0; i < ui->numLayers->value(); ++i)
    {
        GCmd(printer->g, "ACY=200000");   // 200 mm/s^2
        GCmd(printer->g, "DCY=200000");   // 200 mm/s^2
        GCmd(printer->g, "JGY=-25000");   // 15 mm/s jog towards rear limit
        GCmd(printer->g, "BGY");
        GMotionComplete(printer->g, "Y");

        //SECTION 1
        //TURN ON HOPPER
        GCmd(printer->g, "MG{P2} {^85}, {^49}, {^13}{N}");
        GCmd(printer->g, "PRY=115000"); //tune starting point
        GCmd(printer->g, "BGY");
        GMotionComplete(printer->g, "Y");

        //SECTION 2
        //TURN OFF HOPPER
        GCmd(printer->g, "MG{P2} {^85}, {^48}, {^13}{N}");
        GCmd(printer->g, "SB 18");    // Turns on rollers
        GCmd(printer->g, "SB 21");
        GCmd(printer->g, "PRY=115000"); //tune starting point
        GCmd(printer->g, "BGY");
        GMotionComplete(printer->g, "Y");

        GCmd(printer->g, "CB 18");    // Turns off rollers
        GCmd(printer->g, "CB 21");
    }
}


void MainWindow::on_activateRoller1_toggled(bool checked)
{
    std::stringstream s;

    if(checked == 1)
    {
        s << GCmd() << "SB 18" << "\n";
    }
    else
    {
        s << GCmd() << "CB 18" << "\n";
    }

    mPrinterThread->execute_command(s);
}

void MainWindow::on_activateRoller2_toggled(bool checked)
{
    std::stringstream s;

    if(checked == 1)
    {
        s << GCmd() << "SB 21" << "\n";
    }
    else
    {
        s << GCmd() << "CB 21" << "\n";
    }

    mPrinterThread->execute_command(s);
}


void MainWindow::on_activateJet_stateChanged(int arg1)
{
    std::stringstream s;

    if(arg1 == 2)
    {
        s << GCmd() << "SH H"         << "\n"; // enable jetting axis
        s << GCmd() << "ACH=20000000" << "\n"; // set acceleration really high
        s << GCmd() << "JGH=" << 1000 << "\n"; // jetting frequency
        s << GCmd() << "BGH"          << "\n"; // begin jetting
    }
    else
    {
        s << GCmd() << "STH" << "\n"; // stop jetting axis
    }

    mPrinterThread->execute_command(s);
}



