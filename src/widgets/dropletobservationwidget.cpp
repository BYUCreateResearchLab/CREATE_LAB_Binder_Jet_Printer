#include "dropletobservationwidget.h"
#include "ui_dropletobservationwidget.h"

#include <QTimer>

#include "ueye.h"
#include "ueye_tools.h"
#include "subwindow.h"
#include "cameralist.h"
#include "printer.h"
#include "jetdrive.h"

#include <QDebug>

DropletObservationWidget::DropletObservationWidget(JetDrive *jetDrive, QWidget *parent) : PrinterWidget(parent), ui(new Ui::DropletObservationWidget), mJetDrive(jetDrive)
{
    ui->setupUi(this);
    ui->takeVideoButton->setEnabled(false);
    ui->SaveVideoButton->setEnabled(false);
    connect(ui->connectButton, &QPushButton::clicked, this, &DropletObservationWidget::connect_to_camera);
    connect(ui->takeVideoButton, &QPushButton::clicked, this, &DropletObservationWidget::capture_video);
    connect(this, &DropletObservationWidget::video_capture_complete, this, &DropletObservationWidget::stop_avi_capture);
    connect(ui->moveToCameraButton, &QPushButton::clicked, this, &DropletObservationWidget::move_to_jetting_window);
    connect(ui->sweepButton, &QPushButton::clicked, this, &DropletObservationWidget::strobe_sweep_button_clicked);
    connect(ui->TriggerJetButton, &QPushButton::clicked, this, &DropletObservationWidget::trigger_jet_clicked);

    mJettingWidget = new JettingWidget(mJetDrive);
    //ui->frame->layout()->addWidget(mJettingWidget);
    QGridLayout *gridLayout = ui->frame->findChild<QGridLayout*>("gridLayout_frame");
    gridLayout->addWidget(mJettingWidget, 9,0,1,2);
}

DropletObservationWidget::~DropletObservationWidget()
{
    delete ui;
}

void DropletObservationWidget::allow_widget_input(bool allowed)
{
    ui->frame->setEnabled(allowed);

    // DECIDE IF I REALLY WANT THIS...
    if (!allowed && mIsJetting)
    {
        //trigger_jet_clicked(); // will turn off jetting if widget input is disabled and nozzle is currently jetting
        // BUT IT WILL ALSO allow widget input after it is finished...

        // this seems kind of hacky, I need to fix this logic sometime...
        mIsJetting = false;
        ui->TriggerJetButton->setText("Trigger Jet");
    }
}

void DropletObservationWidget::connect_to_camera()
{
    if(mCameraIsConnected)
    {
        ui->mdiArea->activeSubWindow()->close();
    }
    else
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
}

void DropletObservationWidget::camera_closed()
{
    ui->connectButton->setText("Connect Camera");
    ui->takeVideoButton->setEnabled(false);
    ui->SaveVideoButton->setEnabled(false);
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

    allow_widget_input(false);
    mCaptureVideoWithSweep = true;
    strobe_sweep_button_clicked();
}

void DropletObservationWidget::add_frame_to_avi(ImageBufferPtr buffer)
{
    isavi_AddFrame(mAviID, reinterpret_cast<char*>(buffer->data()));
    mNumCapturedFrames++;
    if (mNumCapturedFrames == mNumFramesToCapture)
    {
        // don't add any more frames
        disconnect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
                   this, &DropletObservationWidget::add_frame_to_avi);
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
    this->mCaptureVideoWithSweep = false;
}

void DropletObservationWidget::move_to_jetting_window()
{
    std::stringstream s;
    s << CMD::set_jog(Axis::X, 50);
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::begin_motion(Axis::X);
    s << CMD::display_message("Moving to jetting window");
    s << CMD::motion_complete(Axis::X);

    emit execute_command(s);
    emit disable_user_input();
}

void DropletObservationWidget::strobe_sweep_button_clicked()
{
    // start strobe sweep when a frame is received so that the sweep timing is aligned with image aquisition
    ui->DropletStatsTextEdit->clear();
    connect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
            this, &DropletObservationWidget::start_strobe_sweep);
    mJetDrive->enable_strobe();
}

void DropletObservationWidget::start_strobe_sweep()
{
    // only run this once and then disconnect so when the next frame comes in it doesn't run again
    disconnect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
            this, &DropletObservationWidget::start_strobe_sweep);


    if (mCaptureVideoWithSweep)
    {
        // when a frame is added to the camera, add it to the avi file
        connect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
                this, &DropletObservationWidget::add_frame_to_avi, Qt::DirectConnection);
    }

    int initialDelay = 80; // I need to tune this

    mSweepTimer = new QTimer(this);
    connect(mSweepTimer, &QTimer::timeout, this, &DropletObservationWidget::update_strobe_sweep_offset);
    mSweepTimer->start(initialDelay);

}

void DropletObservationWidget::update_strobe_sweep_offset()
{
    int startStrobeOffset = ui->startTimeSpinBox->value();
    int endStrobeOffset = ui->endTimeSpinBox->value();
    double strobeDelay = 100;
    int stepStrobeOffset = ui->stepTimeSpinBox->value();

    mSweepTimer->setInterval(strobeDelay);
    if (mCurrentStrobeOffset == -1)
    {
        mCurrentStrobeOffset = startStrobeOffset;
        mJetDrive->set_strobe_delay(mCurrentStrobeOffset);
        ui->DropletStatsTextEdit->appendPlainText(QString::number(mCurrentStrobeOffset));
    }
    else if (mCurrentStrobeOffset >= endStrobeOffset)
    {
        disconnect(mSweepTimer, &QTimer::timeout, this, &DropletObservationWidget::update_strobe_sweep_offset);
        mSweepTimer->stop();
        mSweepTimer->deleteLater();
        mCurrentStrobeOffset = -1;
    }
    else
    {
        mCurrentStrobeOffset += stepStrobeOffset;
        mJetDrive->set_strobe_delay(mCurrentStrobeOffset);
        ui->DropletStatsTextEdit->appendPlainText(QString::number(mCurrentStrobeOffset));
    }
}

void DropletObservationWidget::trigger_jet_clicked()
{
    std::stringstream s;
    if (!mIsJetting)
    {
        s << CMD::servo_here(Axis::Jet);
        s << CMD::set_accleration(Axis::Jet, 20000000); // set acceleration really high
        s << CMD::set_jog(Axis::Jet, 1000);             // set to jet at 1000hz
        s << CMD::begin_motion(Axis::Jet);
        mIsJetting = true;
        ui->TriggerJetButton->setText("Stop Jetting");
    }
    else
    {
        s << CMD::stop_motion(Axis::Jet);
        mIsJetting = false;
        ui->TriggerJetButton->setText("Trigger Jet");
    }

    emit execute_command(s);
}

