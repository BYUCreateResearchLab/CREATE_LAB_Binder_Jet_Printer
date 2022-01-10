#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>
#include <thread>
#include "JetServer.h"
#include <QDockWidget>
#include <thread>

using namespace std;

// I am working on updating the 'e' command to give us more verbose output on errors rather than just throwing
// This code is working, but outputs no error on every good command is not verbose about errors. Still working on it...

void MainWindow::e(GReturn rc)
{
    char buf[G_SMALL_BUFFER];
    GError(rc, buf, G_SMALL_BUFFER); //Get Error Information
    //std::cout << buf << '\n';
    if (printer->g)
    {
        GSize size = sizeof(buf);
        GUtility(printer->g, G_UTIL_ERROR_CONTEXT, buf, &size);

        if ((rc == G_BAD_RESPONSE_QUESTION_MARK)
                && (GCommand(printer->g, "TC1", buf, G_SMALL_BUFFER, 0) == G_NO_ERROR))
        {
            //std::cout << buf << '\n'; //Error code from controller
            mOutputWindow->print_string(buf);
        }
    }
}

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
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
    ui->bedSpinBox->setMaximum(300);
    delta_x = 10;
    delta_y = 10;
    delta_z = 15;
    //micronX = ;
    //micronY = ;
    micronZ = 7578;
    //mmX = ;
    //mmY = ;
    mmZ = 757760;
    //Update default GUI values
    MainWindow::on_revertDefault_clicked();

    //Disable all buttons that require a controller connection
    ui->activateHopper->setDisabled(true);
    ui->activateRoller1->setDisabled(true);
    ui->activateRoller2->setDisabled(true);
    ui->activateJet->setDisabled(true);
    ui->xHome->setDisabled(true);
    ui->yHome->setDisabled(true);
    ui->zHome->setDisabled(true);
    ui->xPositive->setDisabled(true);
    ui->yPositive->setDisabled(true);
    ui->xNegative->setDisabled(true);
    ui->yNegative->setDisabled(true);
    ui->zDown->setDisabled(true);
    ui->zUp->setDisabled(true);
    ui->zMax->setDisabled(true);
    ui->zMin->setDisabled(true);
    ui->spreadNewLayer->setDisabled(true);

    sWindow->set_connected(false); // Disable print buttons in line printing window

    mDockWidget = new QDockWidget("Output Window",this);
    this->addDockWidget(Qt::RightDockWidgetArea, mDockWidget);
    mOutputWindow = new OutputWindow(this);
    mDockWidget->setWidget(mOutputWindow);
    mOutputWindow->print_string("Starting Program");
    mOutputWindow->print_string("This is where I want to display messages to the user about what the printer is doing");
    mOutputWindow->print_string("I dont know if I want it to still be a docking window, and what info I want sent here vs displayed as a popup...");
}

void MainWindow::setup(Printer *printerPtr)
{
    printer = printerPtr;
    sWindow->setup(printer);
}

MainWindow::~MainWindow()
{
    // On application close
    delete ui;
    if(printer->g){ // if there is an active connection to a controller
        e(GCmd(printer->g, "MO")); // Turn off the motors
        GClose(printer->g);} // Close the connection to the controller
}


void MainWindow::on_yPositive_clicked()
{
    ui->yPositive->setText("Oh that tickled!");
    ui->xPositive->setText(">");
    ui->yNegative->setText("v");
    ui->xNegative->setText("<");

    int yVel = 1000*ui->yVelocity->value();
    int yDis = 1000*ui->yDistance->value();
    string yVelString = "SPY=" + to_string(yVel);
    string yDisString = "PRY=-" + to_string(yDis);
    if(printer->g){
        e(GCmd(printer->g, "ACY=50000")); // 50 mm/s^2
        e(GCmd(printer->g, "DCY=50000")); // 50 mm/s^2
        e(GCmd(printer->g, yVelString.c_str())); // 10 mm/s
        e(GCmd(printer->g, yDisString.c_str()));  // -10 mm
        e(GCmd(printer->g, "BGY"));
        e(GMotionComplete(printer->g, "Y"));
    }
}

void MainWindow::on_xPositive_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText("Oh that tickled!");
    ui->yNegative->setText("v");
    ui->xNegative->setText("<");

    int xVel = 1000*ui->xVelocity->value();
    int xDis = 1000*ui->xDistance->value();
    string xVelString = "SPX=" + to_string(xVel);
    string xDisString = "PRX=" + to_string(xDis);
    if(printer->g){
        e(GCmd(printer->g, "ACX=800000")); // 800 mm/s^2
        e(GCmd(printer->g, "DCX=800000")); // 800 mm/s^2
        e(GCmd(printer->g, xVelString.c_str()));
        e(GCmd(printer->g, xDisString.c_str()));
        e(GCmd(printer->g, "BGX"));
        e(GMotionComplete(printer->g, "X"));
    }

}

void MainWindow::on_yNegative_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText(">");
    ui->yNegative->setText("Oh that tickled!");
    ui->xNegative->setText("<");

    int yVel = 1000*ui->yVelocity->value();
    int yDis = 1000*ui->yDistance->value();
    string yVelString = "SPY=" + to_string(yVel);
    string yDisString = "PRY=" + to_string(yDis);
    if(printer->g){
        e(GCmd(printer->g, "ACY=50000")); // 50 mm/s^2
        e(GCmd(printer->g, "DCY=50000")); // 50 mm/s^2
        e(GCmd(printer->g, yVelString.c_str()));
        e(GCmd(printer->g, yDisString.c_str()));
        e(GCmd(printer->g, "BGY"));
        e(GMotionComplete(printer->g, "Y"));
    }
}

void MainWindow::on_xNegative_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText(">");
    ui->yNegative->setText("v");
    ui->xNegative->setText("Oh that tickled!");

    int xVel = 1000*ui->xVelocity->value();
    int xDis = 1000*ui->xDistance->value();
    string xVelString = "SPX=" + to_string(xVel);
    string xDisString = "PRX=-" + to_string(xDis);
    if(printer->g){
        e(GCmd(printer->g, "ACX=800000")); // 800 mm/s^2
        e(GCmd(printer->g, "DCX=800000")); // 800 mm/s^2
        e(GCmd(printer->g, xVelString.c_str()));
        e(GCmd(printer->g, xDisString.c_str()));
        e(GCmd(printer->g, "BGX"));
        e(GMotionComplete(printer->g, "X"));
    }

}

void MainWindow::on_xHome_clicked()
{
    //ui->label4Fun->setText("Homing In On X");
    if(printer->g){ // If connected to controller
        // Home the X-Axis using the central home sensor index pulse
        e(GCmd(printer->g, "ACX=800000"));   // 800 mm/s^2
        e(GCmd(printer->g, "DCX=800000"));   // 800 mm/s^2
        e(GCmd(printer->g, "JGX=-15000"));   // 15 mm/s jog towards rear limit
        e(GCmd(printer->g, "BGX"));          // Start motion towards rear limit sensor
        e(GMotionComplete(printer->g, "X")); // Wait until limit is reached
        e(GCmd(printer->g, "JGX=15000"));    // 15 mm/s jog towards home sensor
        e(GCmd(printer->g, "HVX=500"));      // 0.5 mm/s on second move towards home sensor
        e(GCmd(printer->g, "FIX"));          // Find index command for x axis
        e(GCmd(printer->g, "BGX"));          // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
        e(GMotionComplete(printer->g, "X")); // Wait until X stage finishes moving
        e(GCmd(printer->g, "DPX=75000"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
    }
}

void MainWindow::on_yHome_clicked()
{
    ui->label4Fun->setText("Homing In On Y");
    //TODO - HOME Y AXIS ONCE THE LIMIT SENSORS ARE INSTALLED
    if(printer->g){
        e(GCmd(printer->g, "ACY=200000"));   // 200 mm/s^2
        e(GCmd(printer->g, "DCY=200000"));   // 200 mm/s^2
        e(GCmd(printer->g, "JGY=25000"));   // 15 mm/s jog towards rear limit
        e(GCmd(printer->g, "BGY"));          // Start motion towards rear limit sensor
        e(GMotionComplete(printer->g, "Y")); // Wait until limit is reached
        e(GCmd(printer->g, "ACY=50000")); // 50 mm/s^2
        e(GCmd(printer->g, "DCY=50000")); // 50 mm/s^2
        e(GCmd(printer->g, "SPY=25000")); // 25 mm/s
        e(GCmd(printer->g, "PRY=-200000"));  // 201.5 mm
        e(GCmd(printer->g, "BGY"));
        e(GMotionComplete(printer->g, "Y"));
    }
}

void MainWindow::on_zHome_clicked()
{
    ui->label4Fun->setText("Homing In On Z");
    if(printer->g){ // If connected to controller
        // Home the Z-Axis using an offset from the top limit sensor
        e(GCmd(printer->g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
        e(GCmd(printer->g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
        e(GCmd(printer->g, "JGZ=113664"));    // Speed of Z
        try {
            e(GCmd(printer->g, "BGZ")); // Start motion towards rear limit sensor
        } catch(...) {}
        e(GMotionComplete(printer->g, "Z")); // Wait until limit is reached
        e(GCmd(printer->g, "ACZ=757760"));
        e(GCmd(printer->g, "DCZ=757760"));
        e(GCmd(printer->g, "SPZ=113664"));
        e(GCmd(printer->g, "PRZ=-1000000"));//TODO - TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!
        e(GCmd(printer->g, "BGZ"));
        e(GMotionComplete(printer->g, "Z")); // Wait until limit is reached
        e(GCmd(printer->g, "DPZ=0"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
    }
}

void MainWindow::on_zStepSize_valueChanged(int arg1)
{
    delta_z = arg1;
}

void MainWindow::on_zMax_clicked()
{
    z_position = 300;
    ui->bedSpinBox->setValue(z_position);

    e(GCmd(printer->g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
    e(GCmd(printer->g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
    e(GCmd(printer->g, "JGZ=113664"));    // Speed of Z
    try {
        e(GCmd(printer->g, "BGZ")); // Start motion towards rear limit sensor
    } catch(...) {}
    e(GMotionComplete(printer->g, "Z")); // Wait until limit is reached
}

void  MainWindow::on_zUp_clicked()
{

    if(printer->g){
        z_position = z_position + delta_z;
        ui->bedSpinBox->setValue(z_position);
        //DMC Commands : https://www.galil.com/download/comref/com4000/index.html#cover.html+DMC4000

        int zSteps = delta_z*micronZ;
        string zPRZString = "PRZ=" + to_string(zSteps);
        e(GCmd(printer->g, "ACZ=757760"));         //Acceleration of C     757760 steps ~ 1 mm
        e(GCmd(printer->g, "DCZ=757760"));         //Deceleration of C     7578 steps ~ 1 micron
        e(GCmd(printer->g, "SPZ=113664"));         //Speed of C
        e(GCmd(printer->g, zPRZString.c_str()));   //Position Relative of C //HOW TO SWITCH THIS TO GCSTRING IN?
        try
        {
            e(GCmd(printer->g, "BGZ"));            //Begin Motion
        } catch(...) {}
        e(GMotionComplete(printer->g, "Z"));       //Waits until motion is complete?
    }

}

void  MainWindow::on_zDown_clicked()
{

    if(printer->g){
        z_position = z_position - delta_z;
        ui->bedSpinBox->setValue(z_position);

        int zSteps = delta_z*micronZ;
        string zPRZString = "PRZ=-" + to_string(zSteps);
        e(GCmd(printer->g, "ACZ=757760"));
        e(GCmd(printer->g, "DCZ=757760"));
        e(GCmd(printer->g, "SPZ=113664"));
        e(GCmd(printer->g,  zPRZString.c_str()));
        try {
            e(GCmd(printer->g, "BGZ")); // Begin motion
        } catch(...) {}
        e(GMotionComplete(printer->g, "Z"));
    }
}

void  MainWindow::on_zMin_clicked()
{
    z_position = 0;
    ui->bedSpinBox->setValue(z_position);

    e(GCmd(printer->g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
    e(GCmd(printer->g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
    e(GCmd(printer->g, "JGZ=-113664"));    // Speed of Z
    try {
        e(GCmd(printer->g, "BGZ")); // Start motion towards rear limit sensor
    } catch(...) {}
    e(GMotionComplete(printer->g, "Z")); // Wait until limit is reached
}

void MainWindow::on_activateRoller1_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->activateRoller1->setText("Roller Activated!");
    }
    else {
        ui->activateRoller1->setText("Activate Roller");
    }
}

void MainWindow::on_activateRoller2_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->activateRoller2->setText("Roller Activated!");
    }
    else {
        ui->activateRoller2->setText("Activate Roller");
    }
}


void MainWindow::on_activateHopper_stateChanged(int arg1)
{
    if(arg1 == 2)
    {

        ui->activateHopper->setText("Hopper Activated!");
        //ACTIVATE ULTRASONIC GENERATOR
        e(GCmd(printer->g, "MG{P2} {^85}, {^49}, {^13}{N}"));
    }
    else {
        ui->activateHopper->setText("Activate Hopper");
        //DISACTIVATE ULTRASONIC GENERATOR
        e(GCmd(printer->g, "MG{P2} {^85}, {^48}, {^13}{N}"));
    }
}

void MainWindow::on_connect_clicked()
{
    if(printer->g == 0)
    {
        //TO DO - DISABLE BUTTONS UNTIL TUNED, SEPERATE INITIALIZATION HOMING BUTTONS

        //TODO Maybe Threading or something like that? The gui is unresponsive until connect function is finished.
        e(GOpen(printer->address, &printer->g)); // Establish connection with motion controller
        e(GCmd(printer->g, "SH XYZ")); // Enable X,Y, and Z motors

        // Controller Configuration
        e(GCmd(printer->g, "MO")); // Ensure motors are off for setup

        // X Axis
        e(GCmd(printer->g, "MTX = -1"));    // Set motor type to reversed brushless
        e(GCmd(printer->g, "CEX = 2"));     // Set Encoder to reversed quadrature
        e(GCmd(printer->g, "BMX = 40000")); // Set magnetic pitch of lienar motor
        e(GCmd(printer->g, "AGX = 1"));     // Set amplifier gain
        e(GCmd(printer->g, "AUX = 9"));     // Set current loop (based on inductance of motor)
        e(GCmd(printer->g, "TLX = 3"));     // Set constant torque limit to 3V
        e(GCmd(printer->g, "TKX = 0"));     // Disable peak torque setting for now
        // Set PID Settings
        e(GCmd(printer->g, "KDX = 250"));   // Set Derivative
        e(GCmd(printer->g, "KPX = 40"));    // Set Proportional
        e(GCmd(printer->g, "KIX = 2"));     // Set Integral
        e(GCmd(printer->g, "PLX = 0.1"));   // Set low-pass filter

        // Y Axis
        e(GCmd(printer->g, "MTY = 1"));     // Set motor type to standard brushless
        e(GCmd(printer->g, "CEY = 0"));     // Set Encoder to reversed quadrature??? (or is it?)
        e(GCmd(printer->g, "BMY = 2000"));  // Set magnetic pitch of rotary motor
        e(GCmd(printer->g, "AGY = 1"));     // Set amplifier gain
        e(GCmd(printer->g, "AUY = 11"));    // Set current loop (based on inductance of motor)
        e(GCmd(printer->g, "TLY = 6"));     // Set constant torque limit to 6V
        e(GCmd(printer->g, "TKY = 0"));     // Disable peak torque setting for now
        // Set PID Settings
        e(GCmd(printer->g, "KDY = 500"));   // Set Derivative
        e(GCmd(printer->g, "KPY = 70"));    // Set Proportional
        e(GCmd(printer->g, "KIY = 1.7002"));// Set Integral

        // Z Axis
        e(GCmd(printer->g, "MTZ = -2.5"));  // Set motor type to standard brushless
        e(GCmd(printer->g, "CEZ = 14"));    // Set Encoder to reversed quadrature
        e(GCmd(printer->g, "AGZ = 0"));     // Set amplifier gain
        e(GCmd(printer->g, "AUZ = 9"));     // Set current loop (based on inductance of motor)
        // Note: There might be more settings especially for this axis I might want to add later

        // H Axis (Jetting Axis)
        e(GCmd(printer->g, "MTH = -2"));    // Set jetting axis to be stepper motor with defualt low
        e(GCmd(printer->g, "AGH = 0"));     // Set gain to lowest value
        e(GCmd(printer->g, "LDH = 3"));     // Disable limit sensors for H axis
        e(GCmd(printer->g, "KSH = .25"));   // Minimize filters on step signals
        e(GCmd(printer->g, "ITH = 1"));     // Minimize filters on step signals

        e(GCmd(printer->g, "CC 19200, 0, 1, 0")); //AUX PORT FOR THE ULTRASONIC GENERATOR
        e(GCmd(printer->g, "CN= -1"));      // Set correct polarity for all limit switches

        e(GCmd(printer->g, "BN"));          // Save (burn) these settings to the controller just to be safe

        e(GCmd(printer->g, "SH XYZ"));      // Enable X,Y, and Z motors

        //HOME ALL AXIS'
        if(printer->g)
        {
            // Home the X-Axis using the central home sensor index pulse
            e(GCmd(printer->g, "ACX=800000"));   // 800 mm/s^2
            e(GCmd(printer->g, "DCX=800000"));   // 800 mm/s^2
            e(GCmd(printer->g, "SDX=800000"));   // 800 mm/s^2 (deceleration when a limit sensor is activated)
            e(GCmd(printer->g, "JGX=-15000"));   // 15 mm/s jog towards rear limit
            e(GCmd(printer->g, "ACY=200000"));   // 200 mm/s^2
            e(GCmd(printer->g, "DCY=200000"));   // 200 mm/s^2
            e(GCmd(printer->g, "JGY=25000"));   // 15 mm/s jog towards rear limit
            e(GCmd(printer->g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
            e(GCmd(printer->g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
            e(GCmd(printer->g, "JGZ=-113664"));    // Speed of Z
            e(GCmd(printer->g, "FLZ=2147483647")); // Turn off forward limit during homing
            try {
                e(GCmd(printer->g, "BGX"));          // Start motion towards rear limit sensor
            } catch(...) {}
            try {
                e(GCmd(printer->g, "BGY"));          // Start motion towards rear limit sensor
            } catch(...) {}
            try {
                e(GCmd(printer->g, "BGZ")); // Start motion towards rear limit sensor
            } catch(...) {}
            try {
                //Temporary place for Jetting setup to save time
                jetter_setup();
            } catch(...) {}
            e(GMotionComplete(printer->g, "X")); // Wait until limit is reached
            e(GMotionComplete(printer->g, "Y")); // Wait until limit is reached
            e(GMotionComplete(printer->g, "Z")); // Wait until limit is reached
            e(GCmd(printer->g, "JGX=15000"));    // 15 mm/s jog towards home sensor
            e(GCmd(printer->g, "HVX=500"));      // 0.5 mm/s on second move towards home sensor
            e(GCmd(printer->g, "FIX"));          // Find index command for x axis
            e(GCmd(printer->g, "ACY=50000")); // 50 mm/s^2
            e(GCmd(printer->g, "DCY=50000")); // 50 mm/s^2
            e(GCmd(printer->g, "SPY=25000")); // 25 mm/s
            e(GCmd(printer->g, "PRY=-160000"));
            e(GCmd(printer->g, "ACZ=757760"));
            e(GCmd(printer->g, "DCZ=757760"));
            e(GCmd(printer->g, "SDZ=1515520")); // Sets deceleration when limit switch is touched
            e(GCmd(printer->g, "SPZ=113664"));
            e(GCmd(printer->g, "PRZ=1025000"));//TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!
            e(GCmd(printer->g, "BGX"));          // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
            e(GCmd(printer->g, "BGY"));
            e(GCmd(printer->g, "BGZ"));
            e(GMotionComplete(printer->g, "X")); // Wait until X stage finishes moving
            e(GMotionComplete(printer->g, "Y"));
            e(GMotionComplete(printer->g, "Z")); // Wait until limit is reached
            e(GCmd(printer->g, "PRX=-40000"));   //OFFSET TO ACCOUNT FOR THE SKEWED BINDER JET HEAD LOCATION
            e(GCmd(printer->g, "BGX"));
            e(GMotionComplete(printer->g, "X")); // Wait until X stage finishes moving
            e(GCmd(printer->g, "DPX=75000"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
            e(GCmd(printer->g, "DPY=0")); //Do we need this?
            e(GCmd(printer->g, "DPZ=0"));
            e(GCmd(printer->g, "FLZ=0")); // Set software limit on z so it can't go any higher than current position

            ui->connect->setText("Disconnect Controller");
            ui->activateJet->setDisabled(false);

            ui->activateHopper->setDisabled(false);
            ui->activateRoller1->setDisabled(false);
            ui->activateRoller2->setDisabled(false);
            ui->xHome->setDisabled(false);
            ui->yHome->setDisabled(false);
            ui->zHome->setDisabled(false);
            ui->xPositive->setDisabled(false);
            ui->yPositive->setDisabled(false);
            ui->xNegative->setDisabled(false);
            ui->yNegative->setDisabled(false);
            ui->zDown->setDisabled(false);
            ui->zUp->setDisabled(false);
            ui->zMax->setDisabled(false);
            ui->zMin->setDisabled(false);
            ui->spreadNewLayer->setDisabled(false);

            sWindow->set_connected(true); // Enable print buttons in line printing window
        }
    }
    else
    {
        e(GCmd(printer->g, "MO"));       // Disable Motors
        GClose(printer->g);
        printer->g = 0;                  // Reset connection handle
        ui->connect->setText("Connect to Controller");
        ui->activateJet->setDisabled(true);
        ui->activateHopper->setDisabled(true);
        ui->activateRoller1->setDisabled(true);
        ui->activateRoller2->setDisabled(true);
        ui->xHome->setDisabled(true);
        ui->yHome->setDisabled(true);
        ui->zHome->setDisabled(true);
        ui->xPositive->setDisabled(true);
        ui->yPositive->setDisabled(true);
        ui->xNegative->setDisabled(true);
        ui->yNegative->setDisabled(true);
        ui->zDown->setDisabled(true);
        ui->zUp->setDisabled(true);
        ui->zMax->setDisabled(true);
        ui->zMin->setDisabled(true);
        ui->spreadNewLayer->setDisabled(true);

        sWindow->set_connected(false); // Enable print buttons in line printing window
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
    ofs<< "XAxisVelocity\t" << to_string(ui->xVelocity->value()) << "\n";
    ofs<< "YAxisVelocity\t" << to_string(ui->yVelocity->value()) << "\n";
    ofs<< "XAxisDistance\t" << to_string(ui->xDistance->value()) << "\n";
    ofs<< "YAxisDistance\t" << to_string(ui->yDistance->value()) << "\n";
    ofs<< "ZStepSize\t" << to_string(ui->zStepSize->value()) << "\n";
    ofs<< "RollerSpeed\t" << to_string(ui->rollerSpeed->value()) << "\n";
    ofs<< "NumberOfLayers\t" << to_string(ui->numLayers->value()) << "\n";
    ofs.close();
}

void MainWindow::on_revertDefault_clicked()
{
    string line;
    ifstream myfile("C:/Users/ME/Documents/GitHub/CREATE_LAB_Binder_Jet_Printer/PrinterSettings.txt");
    if (myfile.is_open())
    {
        while(getline(myfile, line)) {
            vector<string> row_values;

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
    else cout << "Unable to open file";//Notify user-> "Unable to open file";
}

void MainWindow::on_spreadNewLayer_clicked()
{
    //Spread Layers
    if(printer->g){
        for(int i = 0; i < ui->numLayers->value(); ++i) {
            e(GCmd(printer->g, "ACY=200000"));   // 200 mm/s^2
            e(GCmd(printer->g, "DCY=200000"));   // 200 mm/s^2
            e(GCmd(printer->g, "JGY=-25000"));   // 15 mm/s jog towards rear limit
            e(GCmd(printer->g, "BGY"));
            e(GMotionComplete(printer->g, "Y"));

            //SECTION 1
            //TURN ON HOPPER
            e(GCmd(printer->g, "MG{P2} {^85}, {^49}, {^13}{N}"));
            e(GCmd(printer->g, "PRY=115000")); //tune starting point
            e(GCmd(printer->g, "BGY"));
            e(GMotionComplete(printer->g, "Y"));

            //SECTION 2
            //TURN OFF HOPPER
            e(GCmd(printer->g, "MG{P2} {^85}, {^48}, {^13}{N}"));
            e(GCmd(printer->g, "SB 18"));    // Turns on rollers
            e(GCmd(printer->g, "SB 21"));
            e(GCmd(printer->g, "PRY=115000")); //tune starting point
            e(GCmd(printer->g, "BGY"));
            e(GMotionComplete(printer->g, "Y"));

            e(GCmd(printer->g, "CB 18"));    // Turns off rollers
            e(GCmd(printer->g, "CB 21"));
        }
    }
}


void MainWindow::on_activateRoller1_toggled(bool checked)
{
    if(checked == 1) {
        if(printer->g){
            e(GCmd(printer->g, "SB 18"));
        }
    } else {
        if(printer->g){
            e(GCmd(printer->g, "CB 18"));
        }
    }
}

void MainWindow::on_activateRoller2_toggled(bool checked)
{
    if(checked == 1) {
        if(printer->g){
            e(GCmd(printer->g, "SB 21"));
        }
    } else {
        if(printer->g){
            e(GCmd(printer->g, "CB 21"));
        }
    }
}


void MainWindow::on_activateJet_stateChanged(int arg1)
{
    if(arg1 == 2) {
        e(GCmd(printer->g, "SH H"));
        e(GCmd(printer->g, "ACH=20000000"));   // 200 mm/s^2
        e(GCmd(printer->g, "JGH=1000"));   // 15 mm/s jog towards rear limit
        e(GCmd(printer->g, "BGH"));
    }
    else {
        e(GCmd(printer->g, "STH"));
    }
}



