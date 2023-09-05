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
#include "dropletanalyzer.h"
#include "dropletanalyzerwidget.h"
#include "pressurecontrollerwidget.h"
#include <QSpacerItem>

#include <QDebug>
#include <chrono>

DropletObservationWidget::DropletObservationWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::DropletObservationWidget)
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

    // enable buttons for droplet analysis after video capture has completed
    connect(this, &DropletObservationWidget::video_capture_complete, this, [this](){
        this->ui->openAnalyzerWindowButton->setEnabled(true);
        this->ui->getDropletVelocityButton->setEnabled(true);
        this->ui->detectImageScaleButton->setEnabled(true);
    });

    QGridLayout *gridLayout = ui->frame->findChild<QGridLayout*>("gridLayout_frame");

    // add widgets to the right panel
    m_JettingWidget = new JettingWidget(printer, this);
    gridLayout->addWidget(m_JettingWidget, gridLayout->rowCount(),0,1,-1);

    pressureControllerWidget = new PressureControllerWidget(printer, this);
    gridLayout->addWidget(pressureControllerWidget, gridLayout->rowCount(),0,1,-1);

    // add spacer at bottom (add item takes ownership)
    gridLayout->addItem(new QSpacerItem(20,40,QSizePolicy::Minimum, QSizePolicy::Expanding), gridLayout->rowCount(),0,1,-1);

    m_cameraFrameRate = ui->cameraFPSSpinBox->value();
    m_numFramesToCapture = std::round((ui->endTimeSpinBox->value() - ui->startTimeSpinBox->value()) / ui->stepTimeSpinBox->value()) + 1;
    connect(ui->cameraFPSSpinBox, &QAbstractSpinBox::editingFinished, this, &DropletObservationWidget::framerate_changed);
    connect(ui->shutterAngleSpinBox, &QAbstractSpinBox::editingFinished, this, &DropletObservationWidget::exposure_changed);
    m_tempFileName = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/jetdroplet.avi";
    qDebug() << "temp video files are stored at " << m_tempFileName;
    connect(ui->SaveVideoButton, &QPushButton::clicked, this, &DropletObservationWidget::save_video_clicked);

    m_JetVolumeTimer = new QTimer(this);
    connect(m_JetVolumeTimer, &QTimer::timeout, this, &DropletObservationWidget::end_jet_timer);

    m_ProgressBarTimer = new QTimer(this);
    connect(m_ProgressBarTimer, &QTimer::timeout, this, &DropletObservationWidget::update_progress_bar);   

    ui->jetProgressBar->setMinimum(0);
    ui->jetProgressBar->setMaximum(m_minutesToJet * 60 * 1000);
    ui->jetProgressBar->setValue(0);

    // droplet analyzer objects
    m_analyzer = std::make_unique<DropletAnalyzer>();
    m_analyzerWidget = new DropletAnalyzerWidget(this, m_analyzer.get());
    m_analyzerWindow = std::make_unique<DropletAnalyzerMainWindow>(m_analyzerWidget); // don't set parent so the window shows up in taskbar

    // allow the droplet analyzer and widget to print to the sidebar ouput window
    connect(m_analyzerWidget, &DropletAnalyzerWidget::print_to_output_window, this, &PrinterWidget::print_to_output_window);
    connect(m_analyzer.get(), &DropletAnalyzer::print_to_output_window, this, &PrinterWidget::print_to_output_window);
    setup();
}

DropletObservationWidget::~DropletObservationWidget()
{
    delete ui;
}

void DropletObservationWidget::setup()
{
    ui->imageScaleSpinBox->setValue(m_analyzer->camera_settings().imagePixelSize_um);
    connect(m_analyzerWidget, &DropletAnalyzerWidget::image_scaled_was_changed, this, [this](){this->ui->imageScaleSpinBox->setValue(this->m_analyzer->camera_settings().imagePixelSize_um);});
    connect(ui->imageScaleSpinBox, &QDoubleSpinBox::editingFinished, this, [this](){this->m_analyzerWidget->set_image_scale(this->ui->imageScaleSpinBox->value());});
    //connect(ui->imageScaleSpinBox, &QDoubleSpinBox::editingFinished, m_analyzerWidget, );

    // load video and show widget when button is pressed
    connect(ui->openAnalyzerWindowButton, &QPushButton::clicked, this, [this](){this->show_droplet_analyzer_widget(true);});

    connect(ui->getDropletVelocityButton, &QPushButton::clicked, this, &DropletObservationWidget::calculate_droplet_velocity);
    ui->cameraSettingsFrame->setEnabled(true);

    // enable droplet analyzer widget when analysis is complete
    connect(m_analyzer.get(), &DropletAnalyzer::video_analysis_successful, this, [this](){this->ui->frame->setEnabled(true);});
}

void DropletObservationWidget::allow_widget_input(bool allowed)
{
    ui->frame->setEnabled(allowed);
}

void DropletObservationWidget::jetting_was_turned_on()
{
    m_isJetting = true;
    ui->TriggerJetButton->setText("\nStop Jetting\n");
}

void DropletObservationWidget::jetting_was_turned_off()
{
    m_isJetting = false;
    ui->TriggerJetButton->setText("\nTrigger Jet\n");
}

void DropletObservationWidget::show_droplet_analyzer_widget(bool loadTempVideo)
{
    if (loadTempVideo)
    {
        m_analyzerWidget->load_video_from_observation_widget(m_tempFileName,
                                                             m_JettingWidget->get_jet_drive_settings(),
                                                             ui->stepTimeSpinBox->value());
    }
    m_analyzerWindow.get()->show();
    m_analyzerWindow.get()->activateWindow();
}

void DropletObservationWidget::hide_droplet_analyzer_widget()
{
    m_analyzerWindow.get()->hide();
}

void DropletObservationWidget::calculate_droplet_velocity()
{
    m_analyzerWidget->load_video_from_observation_widget(m_tempFileName,
                                                         m_JettingWidget->get_jet_drive_settings(),
                                                         ui->stepTimeSpinBox->value());
    QMetaObject::invokeMethod(m_analyzer.get(), [this]()
    { this->m_analyzer.get()->analyze_video(); }, Qt::QueuedConnection);

    // disable droplet observation widget settings until analysis is complete
    this->ui->frame->setEnabled(false);

}

bool DropletObservationWidget::is_droplet_anlyzer_window_visible() const
{
    return !m_analyzerWindow.get()->isHidden();
}

void DropletObservationWidget::connect_to_camera()
{
    if (m_cameraIsConnected)
        ui->mdiArea->activeSubWindow()->close();
    else // attempt to connect to camera
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
                // TODO: store a pointer to SubWindow in DropletObservationWidget
                auto *subWindow = new SubWindow(camInfo);
                int numCams = 1;

                // run the following lambda when camera connection opens
                connect(subWindow, &SubWindow::cameraOpenFinished, this, [this, live, subWindow, numCams]() {

                    ui->mdiArea->addSubWindow(subWindow);
                    subWindow->display()->fitToDisplay(true);

                    if (live)
                    {
                        subWindow->camera()->captureVideo(false);
                        // There has to be a better way to get these pointers / shared pointers...
                        m_cameraHandle = subWindow->camera()->handle();
                        m_Camera = subWindow->camera().get();
                    }

                    if (numCams == 1) subWindow->showMaximized();

                    this->set_settings();
                    // enable UI buttons
                    this->ui->takeVideoButton->setEnabled(true);
                    ui->sweepFrame->setEnabled(true);
                    ui->cameraSettingsFrame->setEnabled(true);
                    ui->connectButton->setText("Disconnect Camera");
                    m_cameraIsConnected = true;

                    connect(subWindow, &QWidget::destroyed, this, &DropletObservationWidget::camera_closed);
                });
            }
        }
    }
}

void DropletObservationWidget::camera_closed()
{
    // runs when SubWindow is destroyed (camera is closed)
    m_Camera = nullptr; // no need to delete, this is handled by SubWindow
    m_cameraHandle = 0;
    // update GUI
    ui->connectButton->setText("Connect Camera");
    ui->takeVideoButton->setEnabled(false);
    ui->SaveVideoButton->setEnabled(false);
    ui->sweepFrame->setEnabled(false);
    ui->cameraSettingsFrame->setEnabled(false);
    m_cameraIsConnected = false;
    m_videoHasBeenTaken = false;
}

void DropletObservationWidget::set_settings()
{
    // QRect(x1,y1,x2,y2)
    // TODO: get rid of magic numbers
    m_Camera->aoi.setRect(QRect(680, 0, m_AOIWidth, 2048)); // AOIwidth must be a multiple of 8 for AVI capture
    double fps(m_cameraFrameRate);
    //double exposure_milliseconds{0}; // 0 sets the max possible exposure
    double newFPS{-1.0};
    double newExposure{-1.0};

    // set framerate
    is_SetFrameRate(m_cameraHandle, fps, &newFPS);

    // get exposure range
    double exposureRange[3]; // [0] = min, [1] = max. [2] = increment
    is_Exposure(m_cameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, &exposureRange, sizeof(exposureRange));

    double maxExp = exposureRange[1];
    double desiredExpPercent = ((double)ui->shutterAngleSpinBox->value()) / 360.0;
    double desiredExposure = maxExp * desiredExpPercent;

    // set exposure
    is_Exposure(m_cameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, &desiredExposure, sizeof(desiredExposure));
    // get actual exposure set
    is_Exposure(m_cameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE, &newExposure, sizeof(newExposure));
    ui->cameraFPSSpinBox->setValue(newFPS);
    //qDebug() << QString("The framerate was set as %1 FPS").arg(newFPS);
    //qDebug() << QString("The exposure was set as %1 milliseconds").arg(newExposure);
}

void DropletObservationWidget::capture_video()
{
    isavi_InitAVI(&m_aviID, m_cameraHandle); // initialize AVI capture

    int colorMode{is_SetColorMode(m_cameraHandle, IS_GET_COLOR_MODE)};
    SENSORINFO sensorInfo{};

    is_GetSensorInfo(m_cameraHandle, &sensorInfo);

    int sensorWidth = sensorInfo.nMaxWidth;   // 2048 for our camera
    int sensorHeight = sensorInfo.nMaxHeight; // 2048

    IS_POINT_2D nOffset;
    is_AOI(m_cameraHandle, IS_AOI_IMAGE_GET_POS, &nOffset, sizeof(nOffset));

    // temp offset for cropping the nozzle out of the frame for Colton
    //const int cropPixelAmount {240};
    //const int verticalPixelOffset = ui->cropCheckBox->isChecked() ? cropPixelAmount : 0;
    const int verticalPixelOffset = 0;

    int posX {nOffset.s32X};
    //int posY{nOffset.s32Y}; THIS GOT CHANGED
    int posY{verticalPixelOffset};

    // verticalPixelOffset was added for testing
    isavi_SetImageSize(m_aviID, colorMode, m_AOIWidth, (sensorHeight-verticalPixelOffset), posX, posY, 0); // not sure why a zero here, but it works haha

    int imageQuality {95}; // 1 is the lowest, 100 is the highest
    isavi_SetImageQuality (m_aviID, imageQuality);

    // get temp file location
    std::string fileNameString = m_tempFileName.toStdString();
    const char* fileName = fileNameString.c_str();

    isavi_OpenAVI(m_aviID, fileName);

    int videoFrameRate{m_cameraFrameRate}; // this does not need to be the same as the actual framerate though
    isavi_SetFrameRate(m_aviID, videoFrameRate);
    isavi_StartAVI(m_aviID);

    m_numFramesToCapture = std::round((ui->endTimeSpinBox->value() - ui->startTimeSpinBox->value()) / ui->stepTimeSpinBox->value()) + 1;
    allow_widget_input(false);
    m_captureVideoWithSweep = true;
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
    auto jettingFrequency = ui->jettingFreqSpinBox->value();

    if (!m_isJettingFor3Minutes)
    {
        std::stringstream s;
        if (!m_isJetting)
        {
            s << CMD::set_accleration(Axis::Jet, 20000000); // set acceleration really high
            s << CMD::set_jog(Axis::Jet, jettingFrequency);
            s << CMD::begin_motion(Axis::Jet);
            emit execute_command(s);
            jetting_was_turned_on();
        }

        emit print_to_output_window("Starting 3 minute timer");
        m_isJettingFor3Minutes = true;
        ui->TriggerJetButton->setEnabled(false);
        ui->jetForMinutesButton->setText("Cancel Timer");
        m_JetVolumeTimer->start(m_minutesToJet * 60 * 1000);
        m_ProgressBarTimer->start(2000);
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
    m_JetVolumeTimer->stop();
    m_ProgressBarTimer->stop();
    ui->jetProgressBar->setValue(0);
    m_isJettingFor3Minutes = false;
    ui->jetForMinutesButton->setText("Jet For 3 Minutes");
    emit print_to_output_window("Timer Done");
}

void DropletObservationWidget::update_progress_bar()
{
    int progress = m_JetVolumeTimer->interval() - m_JetVolumeTimer->remainingTime();
    ui->jetProgressBar->setValue(progress);
}

void DropletObservationWidget::start_strobe_sweep()
{
    // TODO: something here very occasionally sometimes causes a crash
    // probably either calling from the wrong thread or
    // a missing mutex lock

    // start strobe sweep
    update_strobe_sweep_offset();

    // update the strobe sweep offset when a new frame is received
    // framereceived from eventthread
    // update must be done from the main thread (accesses ui) (default queued connection)
    connect(m_Camera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
            this, &DropletObservationWidget::update_strobe_sweep_offset);

    if (m_captureVideoWithSweep) // if also capturing a video
    {
        // when a frame is added to the camera, add it to the avi file
        // framereceived signal from event thread
        connect(m_Camera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
                this, &DropletObservationWidget::add_frame_to_avi, Qt::DirectConnection);
    }
}

void DropletObservationWidget::add_frame_to_avi(ImageBufferPtr buffer)
{
    isavi_AddFrame(m_aviID, reinterpret_cast<char*>(buffer->data()));
    m_numCapturedFrames++;
    if (m_numCapturedFrames >= m_numFramesToCapture)
    {
        // don't add any more frames
        disconnect(m_Camera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
                   this, &DropletObservationWidget::add_frame_to_avi);
        emit video_capture_complete();
        unsigned long nLostFrames {777};
        isavi_GetnLostFrames(m_aviID, &nLostFrames);
        qDebug() << nLostFrames << " frames were dropped during video capture";
    }
}

void DropletObservationWidget::stop_avi_capture()
{
    m_numCapturedFrames = 0;
    isavi_StopAVI(m_aviID);
    isavi_CloseAVI(m_aviID);
    isavi_ExitAVI(m_aviID);
    this->allow_widget_input(true);
    this->m_videoHasBeenTaken = true;
    this->ui->SaveVideoButton->setEnabled(true);
    this->m_captureVideoWithSweep = false;
}

void DropletObservationWidget::update_strobe_sweep_offset()
{

    //mSweepTimer->stop();

    if (m_currentStrobeOffset == -1) // if starting strobe sweep
    {
        m_currentStrobeOffset = ui->startTimeSpinBox->value();
        mPrinter->jetDrive->set_strobe_delay(m_currentStrobeOffset);
        const int timeToStepThrough = (ui->endTimeSpinBox->value() - ui->startTimeSpinBox->value());
        ui->sweepProgressBar->setMaximum(timeToStepThrough);
        //emit print_to_output_window(QString::number(mCurrentStrobeOffset));
    }
    else if (m_currentStrobeOffset >= ui->endTimeSpinBox->value()) // if sweep complete
    {
        disconnect(m_Camera, static_cast<void (Camera::*)(ImageBufferPtr)>(&Camera::frameReceived),
                this, &DropletObservationWidget::update_strobe_sweep_offset);
        m_currentStrobeOffset = -1;
        ui->sweepProgressBar->setValue(0);
    }
    else // increment strobe sweep offset
    {
        m_currentStrobeOffset += ui->stepTimeSpinBox->value();
        mPrinter->jetDrive->set_strobe_delay(m_currentStrobeOffset);
        // update progress bar
        ui->sweepProgressBar->setValue(m_currentStrobeOffset - ui->startTimeSpinBox->value());
    }
}

void DropletObservationWidget::trigger_jet_clicked()
{
    auto jettingFrequency = ui->jettingFreqSpinBox->value();

    std::stringstream s;
    if (!m_isJetting)
    {
        s << CMD::servo_here(Axis::Jet);
        s << CMD::set_accleration(Axis::Jet, 20000000); // set acceleration really high
        s << CMD::set_jog(Axis::Jet, jettingFrequency);
        s << CMD::begin_motion(Axis::Jet);
        jetting_was_turned_on();
        ui->jetForMinutesButton->setEnabled(false);
    }
    else
    {
        s << CMD::stop_motion(Axis::Jet);
        jetting_was_turned_off();
        ui->jetForMinutesButton->setEnabled(true);
    }

    emit execute_command(s);
}

void DropletObservationWidget::framerate_changed()
{
    m_cameraFrameRate = ui->cameraFPSSpinBox->value();
    double fps(m_cameraFrameRate);
    double newFPS{-1.0};
    // set framerate
    is_SetFrameRate(m_cameraHandle, fps, &newFPS);
    qDebug() << QString("The framerate was set as %1 FPS").arg(newFPS);
    ui->cameraFPSSpinBox->setValue(newFPS);
    exposure_changed(); // now update the exposure
}

void DropletObservationWidget::exposure_changed()
{
    double exposureRange[3]; // [0] = min, [1] = max. [2] = increment
    is_Exposure(m_cameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, &exposureRange, sizeof(exposureRange));

    //double minExp = exposureRange[0];
    double maxExp = exposureRange[1];
    //double incExp = exposureRange[2];

    double desiredExpPercent = ((double)ui->shutterAngleSpinBox->value()) / 360.0;
    double desiredExposure = maxExp * desiredExpPercent;

    double newExposure{-1.0};
    // set exposure
    is_Exposure(m_cameraHandle, IS_EXPOSURE_CMD_SET_EXPOSURE, &desiredExposure, sizeof(desiredExposure));
    is_Exposure(m_cameraHandle, IS_EXPOSURE_CMD_GET_EXPOSURE, &newExposure, sizeof(newExposure));
    qDebug() << QString("The exposure was set as %1 milliseconds").arg(newExposure);
}

void DropletObservationWidget::save_video_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save as", "", ("Video File (*.avi)"));
    if (!QFile::copy(m_tempFileName, fileName))
    {
        QMessageBox::warning(this, "Warning", "Did not save file");
        return;
    }
}

#include "moc_dropletobservationwidget.cpp"
