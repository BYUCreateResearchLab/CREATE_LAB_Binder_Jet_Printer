#include "dropletobservationwidget.h"
#include "ui_dropletobservationwidget.h"

#include "ueye.h"
#include "subwindow.h"
#include "cameralist.h"

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
    //int numCameras{-1};
    //is_GetNumberOfCameras(&numCameras); // get the number of connected cameras
    //UEYE_CAMERA_LIST cameraList;
    //cameraList.dwCount = numCameras; // tell the list how many cameras are connected
    //qDebug() << QString("There are %1 cameras currently connected").arg(numCameras);

    /*
    HIDS hCam = 0; //Open the first available camera
    int nRet = is_InitCamera(&hCam, NULL);

    if (nRet != IS_SUCCESS)
    {
        //Check if GigE uEye SE needs a new starter firmware
        if (nRet == IS_STARTER_FW_UPLOAD_NEEDED)
        {
            //Calculate time needed for updating the starter firmware
            int nTime;
            is_GetDuration (hCam, IS_SE_STARTER_FW_UPLOAD, &nTime);
       e.g. have progress bar displayed in separate thread

            //Upload new starter firmware during initialization
            hCam = hCam | IS_ALLOW_STARTER_FW_UPLOAD;
            nRet = is_InitCamera(&hCam, NULL);

        end progress bar
        }
    }
    */

    CameraList cameraList;

    if(cameraList.isSingleCamOpenable())
    {
        cameraList.selectAll();
        cameraList.accept();

        auto infoList = cameraList.cameraInfo();
        for (auto& camInfo : infoList)
        {

            bool live = false;
            auto* subWindow = new SubWindow(camInfo);
            auto mdiCount = ui->mdiArea->subWindowList().size();
            int numCams = 1;

            connect(subWindow, &SubWindow::cameraOpenFinished, this, [this, live, subWindow, numCams, mdiCount]() {

                ui->mdiArea->addSubWindow(subWindow);

                if (live)
                {
                    subWindow->camera()->captureVideo(false);
                }

                if (numCams == 1)
                {
                    subWindow->showMaximized();
                }
            });
        }
    }
    else
    {
        if (cameraList.exec() == QDialog::Accepted)
        {
            auto infoList = cameraList.cameraInfo();
            for (auto& camInfo : infoList)
            {
                bool live = false;
                auto* subWindow = new SubWindow(camInfo);
                auto mdiCount = ui->mdiArea->subWindowList().size();
                int numCams = 1;

                connect(subWindow, &SubWindow::cameraOpenFinished, this, [this, live, subWindow, numCams, mdiCount]() {

                    ui->mdiArea->addSubWindow(subWindow);

                    if (live)
                    {
                        subWindow->camera()->captureVideo(false);
                    }

                    if (numCams == 1)
                    {
                        subWindow->showMaximized();
                    }
                });
            }
        }
    }


}


