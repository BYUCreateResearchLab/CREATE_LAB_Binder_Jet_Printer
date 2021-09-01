#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>

using namespace std;

void e(GReturn rc)
 {
   if (rc != G_NO_ERROR)
     throw rc;
 }

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

MainWindow::MainWindow(QMainWindow *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //Set up Second Window
    sWindow = new progWindow();
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

    //Setup Background Image

}

MainWindow::~MainWindow()
{
    // On application close
    delete ui;

    if(g){ // if there is an active connection to a controller
        e(GCmd(g, "MO")); // Turn off the motors
        GClose(g);} // Close the connection to the controller

}


void MainWindow::on_yPositive_clicked()
{
    ui->yPositive->setText("Oh that tickled!");
    ui->xPositive->setText(">");
    ui->yNegative->setText("v");
    ui->xNegative->setText("<");
}

void MainWindow::on_xPositive_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText("Oh that tickled!");
    ui->yNegative->setText("v");
    ui->xNegative->setText("<");

    if(g){
        e(GCmd(g, "ACX=50000")); // 50 mm/s^2
        e(GCmd(g, "DCX=50000")); // 50 mm/s^2
        e(GCmd(g, "SPX=10000")); // 10 mm/s
        e(GCmd(g, "PRX=4000"));  // 4 mm
        e(GCmd(g, "BGX"));
        e(GMotionComplete(g, "X"));
    }

}


void MainWindow::on_yNegative_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText(">");
    ui->yNegative->setText("Oh that tickled!");
    ui->xNegative->setText("<");
}


void MainWindow::on_xNegative_clicked()
{
    ui->yPositive->setText("^");
    ui->xPositive->setText(">");
    ui->yNegative->setText("v");
    ui->xNegative->setText("Oh that tickled!");

    if(g){
        e(GCmd(g, "ACX=50000")); // 50 mm/s^2
        e(GCmd(g, "DCX=50000")); // 50 mm/s^2
        e(GCmd(g, "SPX=10000")); // 10 mm/s
        e(GCmd(g, "PRX=-4000")); // 4 mm
        e(GCmd(g, "BGX"));
        e(GMotionComplete(g, "X"));
    }

}


void MainWindow::on_xHome_clicked()
{
    //ui->label4Fun->setText("Homing In On X");

    if(g){ // If connected to controller
        // Home the X-Axis using the central home sensor index pulse
        e(GCmd(g, "ACX=200000"));   // 200 mm/s^2
        e(GCmd(g, "DCX=200000"));   // 200 mm/s^2
        e(GCmd(g, "JGX=-15000"));   // 15 mm/s jog towards rear limit
        e(GCmd(g, "BGX"));          // Start motion towards rear limit sensor
        e(GMotionComplete(g, "X")); // Wait until limit is reached
        e(GCmd(g, "JGX=15000"));    // 15 mm/s jog towards home sensor
        e(GCmd(g, "HVX=500"));      // 0.5 mm/s on second move towards home sensor
        e(GCmd(g, "FIX"));          // Find index command for x axis
        e(GCmd(g, "BGX"));          // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
        e(GMotionComplete(g, "X")); // Wait until X stage finishes moving
        e(GCmd(g, "DPX=75000"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
    }

}


void MainWindow::on_yHome_clicked()
{
    ui->label4Fun->setText("Homing In On Y");
    //TODO - HOME Y AXIS ONCE THE LIMIT SENSORS ARE INSTALLED
}

void MainWindow::on_zHome_clicked()
{
    ui->label4Fun->setText("Homing In On Y");
    //TODO - HOME Z AXIS ONCE THE LIMIT SENSORS ARE INSTALLED
}


void MainWindow::on_zStepSize_valueChanged(int arg1)
{
    delta_z = arg1;
}


void MainWindow::on_zMax_clicked()
{
    z_position = 300;
    ui->bedSpinBox->setValue(z_position);
}

void  MainWindow::on_zUp_clicked()
{

    if(g){
        z_position = z_position + delta_z;
        ui->bedSpinBox->setValue(z_position);
        //DMC Commands : https://www.galil.com/download/comref/com4000/index.html#cover.html+DMC4000

        int zSteps = delta_z*micronZ;
        string zPRZString = "PRZ=" + to_string(zSteps);
        e(GCmd(g, "ACZ=757760"));         //Acceleration of C     757760 steps ~ 1 mm
        e(GCmd(g, "DCZ=757760"));         //Deceleration of C     7578 steps ~ 1 micron
        e(GCmd(g, "SPZ=113664"));         //Speed of C
        e(GCmd(g, zPRZString.c_str()));   //Position Relative of C //HOW TO SWITCH THIS TO GCSTRING IN?
        e(GCmd(g, "BGZ"));                //Begin Motion
        e(GMotionComplete(g, "Z"));       //Waits until motion is complete?
    }

}

void  MainWindow::on_zDown_clicked()
{

    if(g){
        z_position = z_position - delta_z;
        ui->bedSpinBox->setValue(z_position);

        int zSteps = delta_z*micronZ;
        string zPRZString = "PRZ=-" + to_string(zSteps);
        e(GCmd(g, "ACZ=757760"));
        e(GCmd(g, "DCZ=757760"));
        e(GCmd(g, "SPZ=113664"));
        e(GCmd(g,  zPRZString.c_str()));
        e(GCmd(g, "BGZ"));
        e(GMotionComplete(g, "Z"));
    }


}

void  MainWindow::on_zMin_clicked()
{
    z_position = 0;
    ui->bedSpinBox->setValue(z_position);
}



void MainWindow::on_activateRoller_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->activateRoller->setText("Roller Activated!");
    }
    else {
        ui->activateRoller->setText("Activate Roller");
    }
}


void MainWindow::on_activateHopper_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->activateHopper->setText("Hopper Activated!");
    }
    else {
        ui->activateHopper->setText("Activate Hopper");
    }
}


void MainWindow::on_connect_clicked()
{
    //TO DO - DISABLE BUTTONS UNTIL TUNED, SEPERATE INITIALIZATION HOMING BUTTONS

    if(g==0){
     e(GOpen(address, &g)); // Establish connection with motion controller
     e(GCmd(g, "SH XYZ")); // Enable X,Y, and Z motors
     ui->connect->setText("Disconnect Controller");

     // Controller Configuration
     e(GCmd(g, "MO")); // Ensure motors are off for setup

     // X Axis
     e(GCmd(g, "MTX = -1"));    // Set motor type to reversed brushless
     e(GCmd(g, "CEX = 2"));     // Set Encoder to reversed quadrature
     e(GCmd(g, "BMX = 40000")); // Set magnetic pitch of lienar motor
     e(GCmd(g, "AGX = 1"));     // Set amplifier gain
     e(GCmd(g, "AUX = 9"));     // Set current loop (based on inductance of motor)
     e(GCmd(g, "TLX = 3"));     // Set constant torque limit to 3V
     e(GCmd(g, "TKX = 0"));     // Disable peak torque setting for now

     // Y Axis
     e(GCmd(g, "MTY = 1"));     // Set motor type to standard brushless
     e(GCmd(g, "CEY = 0"));     // Set Encoder to reversed quadrature
     e(GCmd(g, "BMY = 2000"));  // Set magnetic pitch of rotary motor
     e(GCmd(g, "AGY = 1"));     // Set amplifier gain
     e(GCmd(g, "AUY = 11"));    // Set current loop (based on inductance of motor)
     e(GCmd(g, "TLY = 6"));     // Set constant torque limit to 6V
     e(GCmd(g, "TKY = 0"));     // Disable peak torque setting for now

     // Z Axis
     e(GCmd(g, "MTZ = -2.5"));  // Set motor type to standard brushless
     e(GCmd(g, "CEZ = 14"));    // Set Encoder to reversed quadrature
     e(GCmd(g, "AGZ = 0"));     // Set amplifier gain
     e(GCmd(g, "AUZ = 9"));     // Set current loop (based on inductance of motor)
     // Note: There might be more settings especially for this axis I might want to add later

     // H Axis (Jetting Axis)
     e(GCmd(g, "MTH = -2"));    // Set jetting axis to be stepper motor with defualt low
     e(GCmd(g, "AGH = 0"));     // Set gain to lowest value
     e(GCmd(g, "LDH = 3"));     // Disable limit sensors for H axis
     e(GCmd(g, "KSH = .25"));   // Minimize filters on step signals
     e(GCmd(g, "ITH = 1"));     // Minimize filters on step signals

     e(GCmd(g, "BN"));          // Save (burn) these settings to the controller just to be safe

     e(GCmd(g, "SH XYZ"));      // Enable X,Y, and Z motors
     e(GCmd(g, "CN= -1"));      // Set correct polarity for all limit switches

     //HOME TO X&Y AXIS'
     MainWindow::on_xHome_clicked();
     MainWindow::on_yHome_clicked();

     ui->connect->setText("Disconnect Controller");
    }
    else{
        e(GCmd(g, "MO"));       // Disable Motors
        GClose(g);
        g = 0;                  // Reset connection handle
        ui->connect->setText("Connect to Controller");
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




