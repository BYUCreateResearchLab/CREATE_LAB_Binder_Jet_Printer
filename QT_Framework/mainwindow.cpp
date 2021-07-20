#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>

using namespace std;

void e(GReturn rc)
 {
   if (rc != G_NO_ERROR)
     throw rc;
 }

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
    //DOWN THE ROAD -> Update default GUI values

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
        e(GCmd(g, "PRX=-4000"));  // 4 mm
        e(GCmd(g, "BGX"));
        e(GMotionComplete(g, "X"));
    }

}


void MainWindow::on_xHome_clicked()
{
    ui->label4Fun->setText("Homing In On X");

    if(g){ // If connected to controller
        // Home the X-Axis using the central home sensor index pulse
        e(GCmd(g, "ACX=200000")); // 200 mm/s^2
        e(GCmd(g, "DCX=200000")); // 200 mm/s^2
        e(GCmd(g, "JGX=-15000")); // 15 mm/s jog towards rear limit
        e(GCmd(g, "BGX")); // Start motion towards rear limit sensor
        e(GMotionComplete(g, "X")); // Wait until limit is reached
        e(GCmd(g, "JGX=15000")); // 15 mm/s jog towards home sensor
        e(GCmd(g, "HVX=500")); // 0.5 mm/s on second move towards home sensor
        e(GCmd(g, "FIX")); // Find index command for x axis
        e(GCmd(g, "BGX")); // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
        e(GMotionComplete(g, "X"));
        e(GCmd(g, "DPX=75000")); //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
    }
}


void MainWindow::on_yHome_clicked()
{
    ui->label4Fun->setText("Homing In On Y");
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
        /*
        *
        *
        */
        int zSteps = delta_z*micronZ;
        string zPRZString = "PRZ=" + to_string(zSteps);
        e(GCmd(g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
        e(GCmd(g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
        e(GCmd(g, "SPZ=113664"));   //Speed of C
        e(GCmd(g, zPRZString.c_str()));   //Position Relative of C //HOW TO SWITCH THIS TO GCSTRING IN?
        e(GCmd(g, "BGZ"));          //Begin Motion
        e(GMotionComplete(g, "Z")); //Waits until motion is complete?
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


void MainWindow::on_pushButton_2_clicked()
{
    if(g==0){
     e(GOpen(address, &g));
     e(GCmd(g, "SH XYZ")); // Enable X,Y, and Z motors
     ui->pushButton_2->setText("Disconnect Controller");
    }
    else{
        e(GCmd(g, "MO")); // Disable Motors
        GClose(g);
        g = 0; // Reset connection handle
        ui->pushButton_2->setText("Connect to Controller");
    }
}

