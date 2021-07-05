#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->bedSpinBox->setMaximum(300);
    delta_x = 10;
    delta_y = 10;
    delta_z = 15;
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
}

void  MainWindow::on_zDown_clicked()
{
    z_position = z_position - delta_z;
    ui->bedSpinBox->setValue(z_position);
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

