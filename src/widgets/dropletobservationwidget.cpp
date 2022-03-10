#include "dropletobservationwidget.h"
#include "ui_dropletobservationwidget.h"

#include <QTimer>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>

#include "ueye.h"
#include "ueye_tools.h"
#include "subwindow.h"
#include "cameralist.h"
#include "printer.h"
#include "jetdrive.h"

#include <QDebug>
#include <chrono>

DropletObservationWidget::DropletObservationWidget(JetDrive *jetDrive, QWidget *parent) : PrinterWidget(parent), ui(new Ui::DropletObservationWidget), mJetDrive(jetDrive)
{
    ui->setupUi(this);
    setAccessibleName("Droplet Observation Widget");
    ui->takeVideoButton->setEnabled(false);
    ui->SaveVideoButton->setEnabled(false);
    ui->sweepFrame->setEnabled(false);
    ui->cameraSettingsFrame->setEnabled(false);
    connect(ui->connectButton, &QPushButton::clicked, this, &DropletObservationWidget::connect_to_camera);
    connect(ui->takeVideoButton, &QPushButton::clicked, this, &DropletObservationWidget::capture_video);
    connect(this, &DropletObservationWidget::video_capture_complete, this, &DropletObservationWidget::stop_avi_capture);
    connect(ui->moveToCameraButton, &QPushButton::clicked, this, &DropletObservationWidget::move_to_jetting_window);
    connect(ui->moveToMiddleButton, &QPushButton::clicked, this, &DropletObservationWidget::move_towards_middle);
    connect(ui->sweepButton, &QPushButton::clicked, this, &DropletObservationWidget::start_strobe_sweep);
    connect(ui->TriggerJetButton, &QPushButton::clicked, this, &DropletObservationWidget::trigger_jet_clicked);
    connect(ui->jetForMinutesButton, &QPushButton::clicked, this, &DropletObservationWidget::jet_for_three_minutes);

    mJettingWidget = new JettingWidget(mJetDrive);
    QGridLayout *gridLayout = ui->frame->findChild<QGridLayout*>("gridLayout_frame");
    gridLayout->addWidget(mJettingWidget, 23,0,1,3);

    mCameraFrameRate = ui->cameraFPSSpinBox->value();
    mNumFramesToCapture = int(ui->endTimeSpinBox->value() / ui->stepTimeSpinBox->value()) + 1;
    connect(ui->cameraFPSSpinBox, &QAbstractSpinBox::editingFinished, this, &DropletObservationWidget::framerate_changed);
    connect(ui->shutterAngleSpinBox, &QAbstractSpinBox::editingFinished, this, &DropletObservationWidget::exposure_changed);
    mTempFileName = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/jetdroplet.avi";
    qDebug() << "temp video files are stored at " << mTempFileName;
    connect(ui->SaveVideoButton, &QPushButton::clicked, this, &DropletObservationWidget::save_video_clicked);

    mJetVolumeTimer = new QTimer(this);
    connect(mJetVolumeTimer, &QTimer::timeout, this, &DropletObservationWidget::end_jet_timer);

    mProgressBarTimer = new QTimer(this);
    connect(mProgressBarTimer, &QTimer::timeout, this, &DropletObservationWidget::update_progress_bar);
    ui->jetProgressBar->setMinimum(0);
    ui->jetProgressBar->setMaximum(minutesToJet * 60 * 1000);
    ui->jetProgressBar->setValue(0);
}

DropletObservationWidget::~DropletObservationWidget()
{
    delete ui;
}

void DropletObservationWidget::allow_widget_input(bool allowed)
{
    ui->frame->setEnabled(allowed);
}

void DropletObservationWidget::jetting_was_turned_on()
{
        mIsJetting = true;
        ui->TriggerJetButton->setText("\nStop Jetting\n");
}

void DropletObservationWidget::jetting_was_turned_off()
{
    mIsJetting = false;
    ui->TriggerJetButton->setText("\nTrigger Jet\n");
}

void DropletObservationWidget::connect_to_camera()
{
    if (mCameraIsConnected)
    {
        ui->mdiArea->activeSubWindow()->close();
    }
    else
    {
        CameraList cameraList;

        if (cameraList.isSingleCamOpenable())
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
                    subWindow->display()->fitToDisplay(true);

                    if (live)
                    {
                        subWindow->camera()->captureVideo(false);
                        // There has to be a better way to get these pointers / shared pointers...
                        mCameraHandle = subWindow->camera()->handle();
                        mCamera = subWindow->camera().get();
                    }

                    if (numCams == 1) subWindow->showMaximized();

                    this->set_settings();
                    this->ui->takeVideoButton->setEnabled(true);
                    ui->sweepFrame->setEnabled(true);
                    ui->cameraSettingsFrame->setEnabled(true);
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
    mCamera = nullptr;
    mCameraHandle = 0;
    ui->connectButton->setText("Connect Camera");
    ui->takeVideoButton->setEnabled(false);
    ui->SaveVideoButton->setEnabled(false);
    ui->sweepFrame->setEnabled(false);
    ui->cameraSettingsFrame->setEnabled(false);
    mCameraIsConnected = false;
    mVideoHasBeenTaken = false;
}

void DropletObservationWidget::set_settings()
{
    mCamera->aoi.setRect(QRect(680, 0, AOIWidth, 2048)); // AOIwidth must be a multiple of 8 for AVI capture
    double fps(mCameraFrameRate);
    //double exposure_milliseconds{0}; // 0 sets the max possible exposure
    double newFPS{-1.0};
    double newExposure{-1.0};
    // set framerate
    is_SetFrameRate(mCameraHandle, fps, &newFPS);

    // set exposure
    double exposureRange[3]; // [0] = min, [1] = max. [2] = increment
    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, &exposureRange, sizeof(exposureRange));

    double maxExp = exposureRange[1];

    double desiredExpPercent = ((double)ui->shutterAngleSpinBox->value()) / 360.0;
    double desiredExposure = maxExp * desiredExpPercent;

    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, &desiredExposure, sizeof(desiredExposure));
    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE, &newExposure, sizeof(newExposure));
    qDebug() << QString("The framerate was set as %1 FPS").arg(newFPS);
    ui->cameraFPSSpinBox->setValue(newFPS);
    qDebug() << QString("The exposure was set as %1 milliseconds").arg(newExposure);
}

void DropletObservationWidget::capture_video()
{
    isavi_InitAVI(&mAviID, mCameraHandle); // initialize AVI capture

    int colorMode{is_SetColorMode(mCameraHandle, IS_GET_COLOR_MODE)};
    SENSORINFO sensorInfo{};

    is_GetSensorInfo(mCameraHandle, &sensorInfo);

    int sensorWidth = sensorInfo.nMaxWidth;   // 2048 for our camera
    int sensorHeight = sensorInfo.nMaxHeight; // 2048

    IS_POINT_2D nOffset;
    is_AOI(mCameraHandle, IS_AOI_IMAGE_GET_POS, &nOffset, sizeof(nOffset));

    // temp offset for cropping the nozzle out of the frame for Colton
    int verticalPixelOffset{240};

    int posX{nOffset.s32X};
    //int posY{nOffset.s32Y}; THIS GOT CHANGED
    int posY{verticalPixelOffset};

    // verticalPixelOffset was added for testing
    isavi_SetImageSize(mAviID, colorMode, AOIWidth, (sensorHeight-verticalPixelOffset), posX, posY, 0); // not sure why a zero here, but it works haha

    int imageQuality{95}; // 1 is the lowest, 100 is the highest
    isavi_SetImageQuality (mAviID, imageQuality);

    // get temp file location
    std::string fileNameString = mTempFileName.toStdString();
    const char* fileName = fileNameString.c_str();

    isavi_OpenAVI(mAviID, fileName);

    int videoFrameRate{mCameraFrameRate}; // this does not need to be the same as the actual framerate though
    isavi_SetFrameRate(mAviID, videoFrameRate);
    isavi_StartAVI(mAviID);

    mNumFramesToCapture = int(ui->endTimeSpinBox->value() / ui->stepTimeSpinBox->value()) + 1;
    allow_widget_input(false);
    mCaptureVideoWithSweep = true;
    start_strobe_sweep();
}

void DropletObservationWidget::move_to_jetting_window()
{
    std::stringstream s;
    s << CMD::set_speed(Axis::X, 50);
    s << CMD::position_absolute(Axis::X, X_STAGE_LEN_MM);
    //s << CMD::set_jog(Axis::X, 50);
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::begin_motion(Axis::X);
    s << CMD::display_message("Moving to jetting window");
    s << CMD::motion_complete(Axis::X);

    emit execute_command(s);
    emit disable_user_input();
}

void DropletObservationWidget::move_towards_middle()
{
    std::stringstream s;
    s << CMD::set_speed(Axis::X, 50);
    s << CMD::position_absolute(Axis::X, (double)X_STAGE_LEN_MM / 2.0);
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::begin_motion(Axis::X);
    s << CMD::display_message("Moving nozzle to the middle");
    s << CMD::motion_complete(Axis::X);

    emit execute_command(s);
    emit disable_user_input();
}

void DropletObservationWidget::jet_for_three_minutes()
{
    if (!isJettingFor3Minutes)
    {
        std::stringstream s;
        s << CMD::stop_motion(Axis::Jet); // stop jet if it is running
        s << CMD::servo_here(Axis::Jet);
        s << CMD::set_accleration(Axis::Jet, 20000000); // set acceleration really high
        s << CMD::set_jog(Axis::Jet, 1000);             // set to jet at 1000hz
        s << CMD::begin_motion(Axis::Jet);
        emit execute_command(s);
        jetting_was_turned_on();

        emit print_to_output_window("Starting 3 minute timer");

        // toggle the jet after three minutes
        isJettingFor3Minutes = true;
        ui->TriggerJetButton->setEnabled(false);
        ui->jetForMinutesButton->setText("Cancel Timer");
        mJetVolumeTimer->start(minutesToJet * 60 * 1000);
        mProgressBarTimer->start(2000);
    }
    else end_jet_timer();
}

void DropletObservationWidget::end_jet_timer()
{
    std::stringstream s;
    s << CMD::stop_motion(Axis::Jet);
    emit execute_command(s);
    jetting_was_turned_off();

    ui->TriggerJetButton->setEnabled(true);
    mJetVolumeTimer->stop();
    mProgressBarTimer->stop();
    ui->jetProgressBar->setValue(0);
    isJettingFor3Minutes = false;
    ui->jetForMinutesButton->setText("Jet For 3 Minutes");
    emit print_to_output_window("Timer Done");
}

void DropletObservationWidget::update_progress_bar()
{
    int progress = mJetVolumeTimer->interval() - mJetVolumeTimer->remainingTime();
    ui->jetProgressBar->setValue(progress);
}

/*
void DropletObservationWidget::strobe_sweep_button_clicked()
{
    // start strobe sweep when a frame is received so that the sweep timing is aligned with image aquisition
    connect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
            this, &DropletObservationWidget::start_strobe_sweep);
    //mJetDrive->enable_strobe(); this should usually be enabled
}
*/

void DropletObservationWidget::start_strobe_sweep()
{
    // update the strobe sweep offset when a new frame is received
    connect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
            this, &DropletObservationWidget::update_strobe_sweep_offset);

    if (mCaptureVideoWithSweep) // if also capturing a video
    {
        // when a frame is added to the camera, add it to the avi file
        connect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
                this, &DropletObservationWidget::add_frame_to_avi, Qt::DirectConnection);
    }
}

void DropletObservationWidget::add_frame_to_avi(ImageBufferPtr buffer)
{
    isavi_AddFrame(mAviID, reinterpret_cast<char*>(buffer->data()));
    mNumCapturedFrames++;
    if (mNumCapturedFrames >= mNumFramesToCapture)
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

void DropletObservationWidget::start_strobe_sweep_offset_timer()
{
    // delay after frame received to update the strobe offset
    //double frameTime = 1.0 / (double)mCameraFrameRate;
    //double ExposureTimePercent = (double)(ui->shutterAngleSpinBox->value()) / 360.0;
    //int delayTime = (int) (frameTime * ExposureTimePercent);
    //int delayTime = ui->timerTimeSpinBox->value();
    //mSweepTimer->start(delayTime);
}

void DropletObservationWidget::update_strobe_sweep_offset()
{

    //mSweepTimer->stop();

    if (mCurrentStrobeOffset == -1) // if starting strobe sweep
    {
        mCurrentStrobeOffset = ui->startTimeSpinBox->value();
        mJetDrive->set_strobe_delay(mCurrentStrobeOffset);
        int timeToStepThrough = (ui->endTimeSpinBox->value() - ui->startTimeSpinBox->value());
        ui->sweepProgressBar->setMaximum(timeToStepThrough);
        //emit print_to_output_window(QString::number(mCurrentStrobeOffset));
    }
    else if (mCurrentStrobeOffset >= ui->endTimeSpinBox->value()) // if sweep complete
    {
        disconnect(mCamera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
                this, &DropletObservationWidget::update_strobe_sweep_offset);
        mCurrentStrobeOffset = -1;
        ui->sweepProgressBar->setValue(0);
    }
    else // increment strobe sweep offset
    {
        mCurrentStrobeOffset += ui->stepTimeSpinBox->value();
        mJetDrive->set_strobe_delay(mCurrentStrobeOffset);
        // update progress bar
        ui->sweepProgressBar->setValue(mCurrentStrobeOffset - ui->startTimeSpinBox->value());
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
        //mIsJetting = true;
        //ui->TriggerJetButton->setText("Stop Jetting");
        jetting_was_turned_on();
    }
    else
    {
        s << CMD::stop_motion(Axis::Jet);
        //mIsJetting = false;
        //ui->TriggerJetButton->setText("Trigger Jet");
        jetting_was_turned_off();
    }

    emit execute_command(s);
}

void DropletObservationWidget::framerate_changed()
{
    mCameraFrameRate = ui->cameraFPSSpinBox->value();
    double fps(mCameraFrameRate);
    double newFPS{-1.0};
    // set framerate
    is_SetFrameRate(mCameraHandle, fps, &newFPS);
    qDebug() << QString("The framerate was set as %1 FPS").arg(newFPS);
    ui->cameraFPSSpinBox->setValue(newFPS);
    exposure_changed(); // now update the exposure
}

void DropletObservationWidget::exposure_changed()
{
    double exposureRange[3]; // [0] = min, [1] = max. [2] = increment
    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, &exposureRange, sizeof(exposureRange));

    //double minExp = exposureRange[0];
    double maxExp = exposureRange[1];
    //double incExp = exposureRange[2];

    double desiredExpPercent = ((double)ui->shutterAngleSpinBox->value()) / 360.0;
    double desiredExposure = maxExp * desiredExpPercent;

    double newExposure{-1.0};
    // set exposure
    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, &desiredExposure, sizeof(desiredExposure));
    is_Exposure(mCameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE, &newExposure, sizeof(newExposure));
    qDebug() << QString("The exposure was set as %1 milliseconds").arg(newExposure);
}

void DropletObservationWidget::save_video_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save as", "", ("Video File (*.avi)"));
    if (!QFile::copy(mTempFileName, fileName))
    {
        QMessageBox::warning(this, "Warning", "Cannot save file");
        return;
    }
}
