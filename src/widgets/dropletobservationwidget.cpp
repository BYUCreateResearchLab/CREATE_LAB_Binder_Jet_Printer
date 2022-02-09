#include "dropletobservationwidget.h"
#include "ui_dropletobservationwidget.h"

#include "ueye.h"
#include "ueye_tools.h"
#include "subwindow.h"
#include "cameralist.h"

#include <QDebug>

DropletObservationWidget::DropletObservationWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::DropletObservationWidget)
{
    ui->setupUi(this);
    connect(ui->connect, &QPushButton::clicked, this, &DropletObservationWidget::connect_to_camera);
    connect(ui->setSettings, &QPushButton::clicked, this, &DropletObservationWidget::set_settings);
    connect(ui->takeVideo, &QPushButton::clicked, this, &DropletObservationWidget::capture_video);
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

            bool live = true;
            auto* subWindow = new SubWindow(camInfo);
            auto mdiCount = ui->mdiArea->subWindowList().size();
            int numCams = 1;

            connect(subWindow, &SubWindow::cameraOpenFinished, this, [this, live, subWindow, numCams, mdiCount]() {

                ui->mdiArea->addSubWindow(subWindow);

                if (live)
                {
                    subWindow->camera()->captureVideo(false);
                    mCameraHandle = subWindow->camera()->handle();
                    mCamera = subWindow->camera().get();
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
                bool live = true;
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

void DropletObservationWidget::set_settings()
{
    double fps{2};
    double exposure_milliseconds{0}; // 0 sets the max possible exposure
    double newFPS{0};
    double newExposure{-1.0};
    // set framerate
    is_SetFrameRate(mCameraHandle, fps, &newFPS);
    // set exposure
    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, &exposure_milliseconds, sizeof(exposure_milliseconds));
    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE, &newExposure, sizeof(newExposure));
    qDebug() << QString("The framerate was set as %1 FPS").arg(newFPS);
    qDebug() << QString("The exposure was set as %1 milliseconds").arg(newExposure);


}

void DropletObservationWidget::capture_video()
{
    int aviID{0};
    isavi_InitAVI(&aviID, mCameraHandle);

    int colorMode{is_SetColorMode(mCameraHandle, IS_GET_COLOR_MODE)};
    SENSORINFO sensorInfo{};

    is_GetSensorInfo (mCameraHandle, &sensorInfo);

    int sensorWidth = sensorInfo.nMaxWidth;
    int sensorHeight = sensorInfo.nMaxHeight;

    int posX{0};
    int posY{0};
    int lineOffset{0};

    isavi_SetImageSize(aviID, colorMode, sensorWidth, sensorHeight, posX, posY, lineOffset);

    int imageQuality{95}; // 1 is the lowest, 100 is the highest
    isavi_SetImageQuality (aviID, imageQuality);

    const char* fileName = {"C:\\Users\\ME\\Desktop\\testVid.avi"};

    isavi_OpenAVI(aviID, fileName);

    int videoFrameRate{10};

    isavi_SetFrameRate(aviID, videoFrameRate); // this does not need to be the same as the camera frame rate
    isavi_StartAVI(aviID);
    isavi_AddFrame(aviID, mCamera->m_Images[4].pBuf);

    isavi_StopAVI(aviID);
    isavi_CloseAVI(aviID);
    isavi_ExitAVI(aviID);
}


