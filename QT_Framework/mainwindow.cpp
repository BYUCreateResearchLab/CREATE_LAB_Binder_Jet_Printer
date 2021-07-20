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
    delete ui;
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
}


void MainWindow::on_xHome_clicked()
{
    ui->label4Fun->setText("Homing In On X");
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
    z_position = z_position + delta_z;
    ui->bedSpinBox->setValue(z_position);
    //DMC Commands : https://www.galil.com/download/comref/com4000/index.html#cover.html+DMC4000
    /*
     *
     *
    */
    int zSteps = delta_z*micronZ;
    string zPRCString = "PRC=" + to_string(zSteps);
    e(GCmd(g, "MTC=-2.5"));     //Motor Type of C, STANDARD FOR Z AXIS
    e(GCmd(g, "MO"));           //Motor Off (AFFECTS ALL MOTORS)
    e(GCmd(g, "AGC=0"));        //Set Gain of C
    e(GCmd(g, "ACC=757760"));   //Acceleration of C     757760 steps ~ 1 mm
    e(GCmd(g, "DCC=757760"));   //Deceleration of C     7578 steps ~ 1 micron
    e(GCmd(g, "SPC=113664"));   //Speed of C
    e(GCmd(g, zPRCString.c_str()));   //Position Relative of C //HOW TO SWITCH THIS TO GCSTRING IN?
    e(GCmd(g, "SHC"));          //"Servo Here", tells the controller to use the current motor position as the command position and to enable servo control at the current position.
    e(GCmd(g, "BGC"));          //Begin Motion
    e(GMotionComplete(g, "C")); //Waits until motion is complete?
    e(GCmd(g, "MO"));           //Motor Off
}

void  MainWindow::on_zDown_clicked()
{
    z_position = z_position - delta_z;
    ui->bedSpinBox->setValue(z_position);

    int zSteps = delta_z*micronZ;
    string zPRCString = "PRC=-" + to_string(zSteps);
    e(GCmd(g, "MTC=-2.5"));
    e(GCmd(g, "MO"));
    e(GCmd(g, "AGC=0"));
    e(GCmd(g, "ACC=757760"));
    e(GCmd(g, "DCC=757760"));
    e(GCmd(g, "SPC=113664"));
    e(GCmd(g,  zPRCString.c_str()));
    e(GCmd(g, "SHC"));
    e(GCmd(g, "BGC"));
    e(GMotionComplete(g, "C"));
    e(GCmd(g, "MO"));
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
    char const *address = "192.168.42.100";
    e(GOpen(address, &g));
}

