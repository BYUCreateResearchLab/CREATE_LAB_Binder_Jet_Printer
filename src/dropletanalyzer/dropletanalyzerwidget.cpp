#include "ui_dropletanalyzerwidget.h"
#include "dropletanalyzerwidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QStandardPaths>
#include <fstream>

#include "imageviewer.h"
#include "linearanalysis.h"
#include "qcustomplot.h"

//#include "capture.h"
//#include "KDSignalThrottler.h"

DropletAnalyzerWidget::DropletAnalyzerWidget(QWidget *parent, DropletAnalyzer *analyzer) :
    QWidget(parent),
    ui(new Ui::DropletAnalyzerWidget),
    m_analyzer(analyzer)
{
    ui->setupUi(this);
    m_graphWindow = new DropletGraphWindow(this);
    QThread::currentThread()->setObjectName("Main Thread");
    m_view = ui->imageViewer;
    m_view->fitToDisplay(true);
    this->setObjectName("Droplet Analyzer Widget");

    //m_sliderThrottle.reset(new KDToolBox::KDSignalThrottler);
    //m_sliderThrottle->setTimeout(50);

    QObject::connect(m_analyzer, &DropletAnalyzer::image_ready, m_view, &ImageViewer::setImage, Qt::DirectConnection);
    setup();

    //std::cout << cv::getBuildInformation();
}

DropletAnalyzerWidget::~DropletAnalyzerWidget() {}

void DropletAnalyzerWidget::hide_plot_window()
{
    m_graphWindow->hide();
}

void DropletAnalyzerWidget::set_image_scale(double imageScale_um_per_px)
{
    ui->imageScaleSpinBox->setValue(imageScale_um_per_px);
    image_scale_was_edited();
}

void DropletAnalyzerWidget::setup()
{
    nozzle_diameter_was_changed();
    strobe_step_time_was_changed(ui->strobeSweepStepSpinBox->value());
    ui->displaySettingsGroupBox->setEnabled(false);
    ui->analyzeVideoButton->setEnabled(false);
    ui->exportDataButton->setEnabled(false);
    ui->detectLensMagButton->setEnabled(false);

    // startup lens magnification
    ui->magnificationSpinBox->setValue(4);
    m_analyzer->camera_settings().imagePixelSize_um = m_analyzer->camera_settings().image_pixel_size_from_mag(ui->magnificationSpinBox->value());
    ui->imageScaleSpinBox->setValue(m_analyzer->camera_settings().imagePixelSize_um);

    connect(ui->magnificationSpinBox, &QDoubleSpinBox::editingFinished, this, &DropletAnalyzerWidget::magnification_was_edited);
    connect(ui->imageScaleSpinBox, &QDoubleSpinBox::editingFinished, this, &DropletAnalyzerWidget::image_scale_was_edited);
    connect(ui->nozzleTipDiameter, &QDoubleSpinBox::editingFinished, this, &DropletAnalyzerWidget::nozzle_diameter_was_changed);
    connect(ui->strobeSweepStepSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &DropletAnalyzerWidget::strobe_step_time_was_changed);

    connect(ui->detectLensMagButton, &QPushButton::clicked, this, [this](){this->setEnabled(false);});
    connect(ui->detectLensMagButton, &QPushButton::clicked, m_analyzer, &DropletAnalyzer::estimate_image_scale);
    connect(m_analyzer, &DropletAnalyzer::image_scale_estimated, this, &DropletAnalyzerWidget::image_scale_detected);
    connect(m_analyzer, &DropletAnalyzer::image_scale_estimation_failed, this, &DropletAnalyzerWidget::image_scale_detection_failed);
    connect(ui->exportDataButton, &QPushButton::clicked, this, &DropletAnalyzerWidget::export_data_button_pressed);

    connect(ui->loadVideoButton, &QPushButton::clicked, this, &DropletAnalyzerWidget::load_video_button_pressed);
    connect(m_analyzer, &DropletAnalyzer::video_load_failed, this, &DropletAnalyzerWidget::video_load_failed);
    connect(ui->analyzeVideoButton, &QPushButton::clicked, this, [this](){this->setEnabled(false);});
    connect(ui->analyzeVideoButton, &QPushButton::clicked, m_analyzer, &DropletAnalyzer::analyze_video);
    connect(m_analyzer, &DropletAnalyzer::video_analysis_successful, this, &DropletAnalyzerWidget::video_analysis_complete);

    connect(ui->showNozzleOutlineCheckBox, &QCheckBox::clicked, this, &DropletAnalyzerWidget::update_view_settings);
    connect(ui->showProcessedFrameCheckBox, &QCheckBox::clicked, this, &DropletAnalyzerWidget::update_view_settings);
    connect(ui->showDropletContourCheckBox, &QCheckBox::clicked, this, &DropletAnalyzerWidget::update_view_settings);
    connect(ui->showDropletTrackingCheckBox, &QCheckBox::clicked, this, &DropletAnalyzerWidget::update_view_settings);

    connect(m_analyzer, &DropletAnalyzer::video_loaded, this, &DropletAnalyzerWidget::video_loaded);
}

void DropletAnalyzerWidget::reset()
{
    m_analyzer->reset();
    m_view->reset();
    ui->strobeSweepStepSpinBox->setValue(m_analyzer->get_strobe_step_time());
    image_scale_detected();

    ui->videoSettingsGroupBox->setEnabled(true);

    m_videoLoaded = false;

    ui->frameLabel->setText("");
    ui->showDropletContourCheckBox->setChecked(false);
    ui->showDropletTrackingCheckBox->setChecked(false);
    ui->showNozzleOutlineCheckBox->setChecked(false);
    ui->showProcessedFrameCheckBox->setChecked(false);
    ui->strobeSweepStepSpinBox->setEnabled(true);
    ui->displaySettingsGroupBox->setEnabled(false);
    ui->detectLensMagButton->setEnabled(false);

    disconnect(ui->frameSlider, &QSlider::valueChanged, this, &DropletAnalyzerWidget::process_frame_request);
    disconnect(this, &DropletAnalyzerWidget::show_frame, m_analyzer, &DropletAnalyzer::show_frame);

    // reset slider
    ui->frameSlider->setValue(0);
    ui->frameSlider->setRange(0,0);

    update_view_settings(); // tell other thread to not show procssed frame
    ui->analyzeVideoButton->setEnabled(false);
}

void DropletAnalyzerWidget::load_video_from_observation_widget(QString filePath, JetDrive::Settings jetSettings, double strobe_sweep_step_time_us)
{
    reset();
    ui->strobeSweepStepSpinBox->setValue(strobe_sweep_step_time_us);
    strobe_step_time_was_changed(strobe_sweep_step_time_us);
    m_analyzer->set_jetting_settings(jetSettings);

    // load video on thread of analyzer
    QMetaObject::invokeMethod(m_analyzer, [this, filePath]()
    { m_analyzer->load_video(filePath.toStdString()); }, Qt::QueuedConnection);

    // wait until video is loaded
    setEnabled(false);
}

void DropletAnalyzerWidget::export_tracking_data(QString filename)
{
    // ensure extension is .csv
    auto filePath = QFileInfo(filename);
    filename = filePath.absolutePath() + "/" + filePath.baseName() + ".csv";

    std::ofstream file;
    file.open(filename.toStdString());

    file << "DATE_TIME,"
            "IMAGE_SCALE,"
            "STROBE_STEP_TIME,"
            "DROPLET_VELOCITY,"
            "JET_ANGLE,"
            "RISE_TIME_1,DWELL_TIME,FALL_TIME,ECHO_TIME,"
            "RISE_TIME_2,IDLE_VOLTAGE,DWELL_VOLTAGE,ECHO_VOLTAGE"
            "\n";

    file << ","
            "um/px,"
            "us,"
            "m/s,"
            "Rad,"
            "us,us,us,us,"
            "us,V,V,V"
            "\n";

    file << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss").toStdString() << ",";
    file << m_analyzer->camera_settings().imagePixelSize_um << ",";
    file << ui->strobeSweepStepSpinBox->value() << ",";
    file << m_trackingData.velocity_m_s << ",";
    file << m_trackingData.drop_angle_rad << ",";

    std::optional<JetDrive::Settings> jetSettings = m_analyzer->get_jetting_settings();
    if (jetSettings.has_value())
    {
        auto waveform = jetSettings.value().waveform;
        file << waveform.fTRise << ","
             << waveform.fTDwell << ","
             << waveform.fTFall << ","
             << waveform.fTEcho << ","
             << waveform.fTFinal << ","
             << waveform.fUIdle << ","
             << waveform.fUDwell << ","
             << waveform.fUEcho;
    }
    file << "\n";


    // write tracking data to .csv file
    file << "T,X,Y\n";
    file << "us,um,um\n";

    for (size_t i{0}; i<m_trackingData.t.size(); i++)
    {
        file << m_trackingData.t[i]
                << "," << m_trackingData.x[i]
                   << "," << m_trackingData.y[i] << "\n";
    }


    file.close();
}

void DropletAnalyzerWidget::magnification_was_edited()
{
    const double numDecimals = ui->magnificationSpinBox->decimals();
    if (qAbs(ui->magnificationSpinBox->value()
             - m_analyzer->camera_settings().calculate_lens_magnification())
            < pow(10.0, -numDecimals))
        return;
    else
    {
        qDebug() << "magnification value was different enough";
        m_analyzer->camera_settings().imagePixelSize_um = m_analyzer->camera_settings().image_pixel_size_from_mag(ui->magnificationSpinBox->value());
        ui->imageScaleSpinBox->setValue(m_analyzer->camera_settings().imagePixelSize_um);
        emit image_scaled_was_changed();
    }
}

void DropletAnalyzerWidget::image_scale_was_edited()
{
    // don't update things if the current value is within rounding of the stored setting
    const double numDecimals = ui->imageScaleSpinBox->decimals();
    if (qAbs(ui->imageScaleSpinBox->value()
             - m_analyzer->camera_settings().imagePixelSize_um)
            < pow(10.0, -numDecimals))
        return;
    else
    {
        qDebug() << "image scale value was different enough";
        m_analyzer->camera_settings().imagePixelSize_um = ui->imageScaleSpinBox->value();
        ui->magnificationSpinBox->setValue(m_analyzer->camera_settings().calculate_lens_magnification());
        emit image_scaled_was_changed();
    }
}

void DropletAnalyzerWidget::nozzle_diameter_was_changed()
{
    m_analyzer->set_nozzle_diameter(ui->nozzleTipDiameter->value());
}

void DropletAnalyzerWidget::strobe_step_time_was_changed(int stepTime_us)
{
    m_analyzer->set_strobe_step_time(stepTime_us);
}

void DropletAnalyzerWidget::set_frame_range(int numFrames)
{
    // emit print_to_output_window(QString("There are %1 frames in the video").arg(numFrames));
    ui->frameSlider->setRange(0, numFrames-1);
    ui->frameSlider->setValue(0);
}

void DropletAnalyzerWidget::update_view_settings()
{
    const int frame = ui->frameSlider->value();

    DropViewSettings viewSettings;
    viewSettings.showProcessedFrame = ui->showProcessedFrameCheckBox->isChecked();
    viewSettings.showDropletContour = ui->showDropletContourCheckBox->isChecked();
    viewSettings.showNozzleOutline = ui->showNozzleOutlineCheckBox->isChecked();
    viewSettings.showTracking = ui->showDropletTrackingCheckBox->isChecked();

    m_analyzer->update_view_settings(viewSettings);

    if (m_videoLoaded) process_frame_request(frame);
}

void DropletAnalyzerWidget::image_scale_detected()
{
    this->setEnabled(true);
    ui->imageScaleSpinBox->setValue(m_analyzer->camera_settings().imagePixelSize_um);
    ui->magnificationSpinBox->setValue(m_analyzer->camera_settings().calculate_lens_magnification());
    emit image_scaled_was_changed();
}

void DropletAnalyzerWidget::image_scale_detection_failed()
{
    this->setEnabled(true);
    ui->analyzeVideoButton->setFocus();
    ui->detectLensMagButton->setEnabled(false);
    emit print_to_output_window("Could not detect nozzle in video");
}

void DropletAnalyzerWidget::load_video_button_pressed()
{
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filename = QFileDialog::getOpenFileName(this, "Open File", defaultDir, ("Video (*.avi)"));

    if(!filename.isEmpty() && !filename.isNull())
    {
        // check to make sure file isn't huge and is a video file
        const int maxFileSizeBytes = 500000000;
        QFileInfo file(filename);
        if (file.size() > (maxFileSizeBytes) ||
           (file.suffix() != "avi" && file.suffix() != "mp4"))
        {
            emit print_to_output_window("Invalid file or file is too large");
            emit print_to_output_window(QString("Max file size is %1 MB").arg(maxFileSizeBytes / 1000000));
            return;
        }


        reset();
        // load video on thread of analyzer
        QMetaObject::invokeMethod(m_analyzer, [this, filename]()
        { m_analyzer->load_video(filename.toStdString()); }, Qt::QueuedConnection);

        // wait until video is loaded
        setEnabled(false);
    }
}

void DropletAnalyzerWidget::video_loaded()
{
    m_videoLoaded = true;

    set_frame_range(m_analyzer->get_number_of_frames());

    // MAKE SURE TO DISCONNECT THESE ON RESET
    connect(ui->frameSlider, &QSlider::valueChanged, this, &DropletAnalyzerWidget::process_frame_request);
    connect(this, &DropletAnalyzerWidget::show_frame, m_analyzer, &DropletAnalyzer::show_frame);

    ui->frameSlider->setValue(0);
    process_frame_request(0);

    this->setEnabled(true);
    ui->analyzeVideoButton->setEnabled(true);
    ui->detectLensMagButton->setEnabled(true);
}

void DropletAnalyzerWidget::video_load_failed()
{
    this->setEnabled(true);
    emit print_to_output_window("video failed to load");
}

void DropletAnalyzerWidget::video_analysis_complete()
{
    ui->showDropletTrackingCheckBox->setChecked(true);
    update_view_settings();

    this->setEnabled(true);
    ui->displaySettingsGroupBox->setEnabled(true);
    ui->exportDataButton->setEnabled(true);

    // plot data
    m_trackingData = m_analyzer->get_droplet_tracking_data();
    const auto& dataX = m_trackingData.t;
    const auto& dataY = m_trackingData.y;

    LinearAnalysis::FitLine fitLine;
    fitLine.intercept = m_trackingData.intercept;
    fitLine.slope = m_trackingData.velocity_m_s;
    auto fitY = LinearAnalysis::get_fit_vals(fitLine, dataX);
    const QString text = QString("Droplet Velocity: %1 m/s").arg(m_trackingData.velocity_m_s, 0, 'f', 3);
    emit print_to_output_window(text);
    ui->outputTextEdit->appendPlainText(text);

    m_graphWindow->plot_data(dataX, dataY);
    m_graphWindow->plot_trend_line(dataX, fitY);

    if (!isHidden()) m_graphWindow->show();

    // use this if you want to have Qt delete the graph window when it is closed, otherwise it is just hidden
    //m_graphWindow->setAttribute(Qt::WA_DeleteOnClose);
}

void DropletAnalyzerWidget::process_frame_request(int frameNum)
{
    QString labelText = "Frame " + QString::number(frameNum + 1);
    ui->frameLabel->setText(labelText);
    emit show_frame(frameNum);
}

void DropletAnalyzerWidget::export_data_button_pressed()
{
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filename = QFileDialog::getSaveFileName(this, "Save Tracking Data", defaultDir, ("csv (*.csv)"));

    if(!filename.isEmpty() && !filename.isNull())
    {
        export_tracking_data(filename);
    }
}


DropletGraphWindow::DropletGraphWindow(QWidget *parent) :
    QMainWindow(parent)
{
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
    m_verticalLayout = new QVBoxLayout(m_centralWidget);
    m_verticalLayout->setSpacing(6);
    m_verticalLayout->setContentsMargins(11, 11, 11, 11);
    m_verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    m_plot = new QCustomPlot(this);

    m_verticalLayout->addWidget(m_plot);

    setCentralWidget(m_centralWidget);

    setGeometry(400, 250, 542, 390);
    setWindowTitle("Droplet Tracking");
}

void DropletGraphWindow::plot_data(const std::vector<double> &x, const std::vector<double> &y)
{
    // add two new graphs and set their look:
    m_plot->addGraph(); // graph 0
    m_plot->graph(0)->setPen(QPen(Qt::black)); // line color blue for first graph
    //graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue

    m_plot->axisRect()->setupFullAxesBox(true);

    // pass data points to graphs:
    m_plot->graph(0)->setData(QVector<double>(x.begin(), x.end()),
                              QVector<double>(y.begin(), y.end()));

    m_plot->xAxis->setLabel("Time (µs)");
    m_plot->yAxis->setLabel("Distance (µm)");

    // set scatter style
    m_plot->graph()->setScatterStyle(QCPScatterStyle::ssPlus);

    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    m_plot->graph(0)->rescaleAxes();

    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    m_plot->replot();
}

void DropletGraphWindow::plot_trend_line(const std::vector<double> &x, const std::vector<double> &y)
{
    m_plot->addGraph(); // graph 0
    m_plot->graph(1)->setPen(QPen(Qt::red));


    m_plot->graph(1)->setData(QVector<double>(x.begin(), x.end()),
                              QVector<double>(y.begin(), y.end()));
    m_plot->graph(1)->setScatterStyle(QCPScatterStyle::ssNone);
    m_plot->replot();
}


DropletAnalyzerMainWindow::DropletAnalyzerMainWindow(DropletAnalyzerWidget* analyzerWidget, QWidget *parent) :
    QMainWindow(parent),
    m_dropletAnalyzerWidget(analyzerWidget)
{
    setCentralWidget(m_dropletAnalyzerWidget);

    setObjectName("DropletAnalyzerWindow");
    setGeometry(1000, 100, 300, 1000);
    setWindowTitle("Droplet Analyzer");

    //QSize size = QGuiApplication::primaryScreen()->availableGeometry().size();
    //resize(size.height() * .45, size.height() * .75);
}

DropletAnalyzerMainWindow::~DropletAnalyzerMainWindow()
{

}

void DropletAnalyzerMainWindow::closeEvent (QCloseEvent *event)
{
    m_dropletAnalyzerWidget->hide_plot_window();
    m_dropletAnalyzerWidget->reset();

}

#include "moc_dropletanalyzerwidget.cpp"
