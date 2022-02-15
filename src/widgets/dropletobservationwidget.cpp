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
    ui->takeVideoButton->setEnabled(false);
    ui->SaveVideoButton->setEnabled(false);
    connect(ui->connectButton, &QPushButton::clicked, this, &DropletObservationWidget::connect_to_camera);
    connect(ui->takeVideoButton, &QPushButton::clicked, this, &DropletObservationWidget::capture_video);
    connect(this, &DropletObservationWidget::video_capture_complete, this, &DropletObservationWidget::stop_avi_capture);
}

DropletObservationWidget::~DropletObservationWidget()
{
    delete ui;
}

void DropletObservationWidget::allow_widget_input(bool allowed)
{
    ui->frame->setEnabled(allowed);
    //ui->moveToCameraButton->setEnabled(allowed);
    //ui->TriggerJetButton->setEnabled(allowed);
}

void DropletObservationWidget::connect_to_camera()
{

    CameraList cameraList;

    if(cameraList.isSingleCamOpenable())
    {
        cameraList.selectAll();
        cameraList.accept();

        auto infoList = cameraList.cameraInfo();
        for (auto &camInfo : infoList)
        {
            bool live = true;
            auto *subWindow = new SubWindow(camInfo);
            int numCams = 1;

            connect(subWindow, &SubWindow::cameraOpenFinished, this, [this, live, subWindow, numCams]() {

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

                this->set_settings();
                this->ui->takeVideoButton->setEnabled(true);
                ui->connectButton->setText("Disconnect Camera");
                mCameraIsConnected = true;

                connect(subWindow, &QWidget::destroyed, this, &DropletObservationWidget::camera_closed);
            });
        }
    }
}

void DropletObservationWidget::camera_closed()
{
    ui->connectButton->setText("Connect Camera");
    mCameraIsConnected = false;
    mVideoHasBeenTaken = false;
}

void DropletObservationWidget::set_settings()
{
    double fps{10};
    double exposure_milliseconds{0}; // 0 sets the max possible exposure
    double newFPS{-1.0};
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
    isavi_InitAVI(&mAviID, mCameraHandle);

    int colorMode{is_SetColorMode(mCameraHandle, IS_GET_COLOR_MODE)};
    SENSORINFO sensorInfo{};

    is_GetSensorInfo (mCameraHandle, &sensorInfo);

    int sensorWidth = sensorInfo.nMaxWidth;
    int sensorHeight = sensorInfo.nMaxHeight;

    int posX{0};
    int posY{0};
    int lineOffset{0};

    isavi_SetImageSize(mAviID, colorMode, sensorWidth, sensorHeight, posX, posY, lineOffset);

    int imageQuality{95}; // 1 is the lowest, 100 is the highest
    isavi_SetImageQuality (mAviID, imageQuality);

    std::string fileNameString{getenv("USERPROFILE")};
    fileNameString += "/Desktop/testVid.avi";
    const char* fileName = fileNameString.c_str();

    isavi_OpenAVI(mAviID, fileName);

    int videoFrameRate{10};

    isavi_SetFrameRate(mAviID, videoFrameRate); // this does not need to be the same as the camera frame rate
    isavi_StartAVI(mAviID);


    //isavi_AddFrame(mAviID, mCamera->m_Images[4].pBuf);
    // connect to the camera signal such that when a new frame is added, the frame is sent to isavi_AddFrame();

    // once the desired number of frames have been captured, break the link and stop the video

    // NOT THIS ONE...
    //connect(this, SELECT<>::OVERLOAD_OF(&Camera::frameReceived), this, &Camera::processCurrentImageInMemory, Qt::DirectConnection);
    // THIS ONE?

    // when a frame is added to the camera, add it to the avi file
    connect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived), this, &DropletObservationWidget::add_frame_to_avi, Qt::DirectConnection);
    allow_widget_input(false);
}

void DropletObservationWidget::add_frame_to_avi(ImageBufferPtr buffer)
{
    isavi_AddFrame(mAviID, reinterpret_cast<char*>(buffer->data()));
    mNumCapturedFrames++;
    if (mNumCapturedFrames == mNumFramesToCapture)
    {
        // don't add any more frames
        disconnect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived), this, &DropletObservationWidget::add_frame_to_avi);
        emit video_capture_complete();
        unsigned long nLostFrames{777};
        isavi_GetnLostFrames(mAviID, &nLostFrames);
        qDebug() << nLostFrames << " frames were dropped during video capture";
    }
}

void DropletObservationWidget::stop_avi_capture()
{
    mNumCapturedFrames = 0;
    isavi_StopAVI(mAviID);
    isavi_CloseAVI(mAviID);
    isavi_ExitAVI(mAviID);
    this->allow_widget_input(true);
    this->mVideoHasBeenTaken = true;
    this->ui->SaveVideoButton->setEnabled(true);
}


