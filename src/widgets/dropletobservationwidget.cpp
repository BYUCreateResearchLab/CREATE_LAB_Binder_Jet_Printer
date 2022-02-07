#include "dropletobservationwidget.h"
#include "ui_dropletobservationwidget.h"

#include "ueye.h"

#include <QDebug>

DropletObservationWidget::DropletObservationWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::DropletObservationWidget)
{
    ui->setupUi(this);
    connect(ui->connect, &QPushButton::clicked, this, &DropletObservationWidget::connect_to_camera);
}

DropletObservationWidget::~DropletObservationWidget()
{
    delete ui;
}

void DropletObservationWidget::allow_widget_input(bool allowed)
{

}

void DropletObservationWidget::connect_to_camera()
{
    int numCameras{-1};
    is_GetNumberOfCameras(&numCameras);
    qDebug() << QString("There are %1 cameras currently connected").arg(numCameras);
}


