#include "bedmicroscopewidget.h"
#include "ui_bedmicroscopewidget.h"
#include "printer.h"
#include "display.h"
#include "bedmicroscope.h"
#include "dmc4080.h"

#include <QImage>
#include <QPainter>
#include <QFileDialog>
#include <QDebug>

class MicroscopeWorker : public QObject
{
    Q_OBJECT

public:
    MicroscopeWorker(Printer *printer_):
    mPrinter(printer_)
    {

    }

public slots:

    void stop()
    {
        QMutexLocker locker(&m_mutex);
        m_stopRequested = true;
    }

    void process()
    {

        while (true)
        {
            if (mPrinter->bedMicroscope->is_connected())
            {
                QImage image = mPrinter->bedMicroscope->get_frame();
                emit imageReady(image);
            }
            QThread::msleep(40); // Sleep for 40 milliseconds
            if (isStopped()) break;
        }
        m_stopRequested = false; // reset
    }

signals:
    void imageReady(const QImage& image);

private:
    Printer* mPrinter {nullptr};

    QMutex m_mutex;
    bool m_stopRequested = false;

    bool isStopped()
    {
        QMutexLocker locker(&m_mutex);
        return m_stopRequested;
    }
};


//QImage createImage() {
//    // Create a white image
//    QImage image(640, 320, QImage::Format_RGB888);
//    image.fill(Qt::white);

//    // Draw a black circle in the middle
//    QPainter painter(&image);
//    painter.setPen(Qt::black);
//    painter.setBrush(Qt::black);
//    painter.drawEllipse(QPoint(320, 160), 100, 100); // Circle at center (320, 160) with radius 100

//    return image;
//}

BedMicroscopeWidget::BedMicroscopeWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::BedMicroscopeWidget),
    timer(new QTimer(this))
{

    ui->setupUi(this);

    connect(ui->connectButton, &QPushButton::clicked, this, &BedMicroscopeWidget::connect_to_camera);
    connect(ui->setSaveFolderButton, &QPushButton::clicked, this, &BedMicroscopeWidget::set_save_folder);
    connect(ui->captureImagesButton, &QPushButton::clicked, this, &BedMicroscopeWidget::capture_images);

//    update_display(createImage());
}

BedMicroscopeWidget::~BedMicroscopeWidget()
{
    delete ui;
}

void BedMicroscopeWidget::allow_widget_input(bool allowed)
{
    ui->frame->setEnabled(allowed);
}

void BedMicroscopeWidget::connect_to_camera()
{
    if (!mPrinter->bedMicroscope->is_connected())
    {
        QString selectedItemText = ui->cameraIDComboBox->currentText();
        bool conversionOk;
        int index = selectedItemText.toInt(&conversionOk);
        bool connected {false};
        if (!conversionOk) index = 0;
        connected = mPrinter->bedMicroscope->open_capture(index);
        // start timer if connected
        if (connected)
        {
            workerThread = new QThread(this);
            worker = new MicroscopeWorker(mPrinter);
            worker->moveToThread(workerThread);
            connect(workerThread, &QThread::started, worker, &MicroscopeWorker::process);
            connect(worker, &MicroscopeWorker::imageReady, this, &BedMicroscopeWidget::update_display, Qt::DirectConnection);
            ui->connectButton->setText("Disconnect");
            workerThread->start();
        }
        else
        {
            emit print_to_output_window("Could not connect to microscope camera");
        }
    }
    else // already connected
    {
        if (worker)
        {
            worker->stop(); // Stop the worker
            workerThread->quit(); // Quit the worker thread
            workerThread->wait(); // Wait for the worker thread to finish
            delete worker; // Delete the worker object
            delete workerThread; // Delete the thread object
            worker = nullptr;
            workerThread = nullptr;
        }
        ui->imageDisplay->reset();
        mPrinter->bedMicroscope->disconnect_camera();
        ui->connectButton->setText("Connect to Microscope Camera");
    }


}

void BedMicroscopeWidget::update_display(const QImage &image)
{
    ui->imageDisplay->setImage(image);
}

void BedMicroscopeWidget::show_microscope_image()
{
    if (mPrinter->bedMicroscope->is_connected())
    {
        update_display(mPrinter->bedMicroscope->get_frame());
    }
}

void BedMicroscopeWidget::set_save_folder()
{
    // Open file dialog to select a folder
    QString folderPath = QFileDialog::getExistingDirectory(nullptr, "Select Folder", QDir::homePath());

    if (!folderPath.isEmpty())
    {
        saveFolderPath = folderPath;
        ui->saveFolderLabel->setText(saveFolderPath);
    }
}

void BedMicroscopeWidget::capture_images()
{
    if (!mPrinter->bedMicroscope->is_connected() || saveFolderPath.isEmpty()) { return; }

    // generate dmc file to move printer to positions
    std::stringstream s;
    s << "## Bed imaging DMC program on the BJ system\n";
    s << "Jacob Lawrence\n";
    s << "##option \"--min 4\"\n\n";

    s << "#BEGIN;\n";
    s << "JS #store;\n"; // initialize arrays
    s << "SP " << CMD::detail::mm2cnts(50, Axis::X) << ", " << CMD::detail::mm2cnts(30, Axis::Y) << "\n"; // set speed
    s << "AC " << CMD::detail::mm2cnts(5000, Axis::X) << ", " << CMD::detail::mm2cnts(1000, Axis::Y) << "\n"; // set acceleration
    s << "DC " << CMD::detail::mm2cnts(5000, Axis::X) << ", " << CMD::detail::mm2cnts(1000, Axis::Y) << "\n"; // set deceleration
    s << "\n";
    s << "nx = 0;\n";
    s << "ny = 0;\n";

    s << "#loopY;\n";
    s << "PAY = yPos[ny];\n"; // move to y position
    s << "#loopX;\n";
    s << "PAX = xPos[nx];\n"; // move to x position
    s << "BG XY;\n"; // start motion
    s << "AM XY;\n"; // after motion complete
    s << "WT 500;\n"; // wait for 0.5 seconds
    s << "nxStr = (97+nx)*$1000000;\n";
    s << "MG \"CMD MICRO_CAP \", nxStr{S1}, (ny+1){Z2.0}\n"; // request image
//    s << "MG \"CMD MICRO_CAP \" {^(97+nx)}, (ny+1){Z1.0}\n"; // request image
    s << "WT 1000;\n"; // wait for 1 second
    s << "nx = nx + 1\n";
    s << "JP #loopX, (nx < " << ui->numXSpinBox->value() << ");\n";
    s << "nx = 0;\n"; // reset x variable
    s << "ny = ny + 1\n";
    s << "JP #loopY, (ny < " << ui->numYSpinBox->value() << ");\n";
    s << "EN;\n";
    s << "\n\n";

    s << "#store\n";
    s << "DA *,*[0];\n"; //Deallocate all variables and all arrays
    s << "DM xPos[" << ui->numXSpinBox->value() << "];\n"; // define x array
    s << "DM yPos[" << ui->numYSpinBox->value() << "];\n"; // define x array
    s << "\n";
    // populate x-values
    for (int i = 0; i < ui->numXSpinBox->value(); ++i)
    {
        int xPos = CMD::detail::mm2cnts(ui->xStartPositionSpinBox->value() + (i * ui->xSpacingSpinBox->value()), Axis::X);
        s << "xPos[" << i << "] = " << xPos << ";\n";
    }
    s << "\n";
    // populate y-values
    for (int i = 0; i < ui->numYSpinBox->value(); ++i)
    {
        int yPos = CMD::detail::mm2cnts(ui->yStartPositionSpinBox->value() + (i * ui->ySpacingSpinBox->value()), Axis::Y);
        s << "yPos[" << i << "] = " << yPos << ";\n";
    }
    s << "EN;\n"; // end subroutine





    std::string temp = s.str();
    const char *program = temp.c_str();

//    qDebug().noquote() << QString::fromStdString(temp); // debugging

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, program, "--max 4");
    }
    std::stringstream s2;
    s2 << "GCmd," << "XQ #BEGIN" << "\n";
    s2 << "GProgramComplete," << "\n";

    emit print_to_output_window(QString("Imaging Bed"));

    emit execute_command(s2);
    emit disable_user_input();
}

void BedMicroscopeWidget::export_image(const QString &position)
{
    int bedID = ui->bedIDSpinBox->value();
    QString fileName = QString("%1/%2_%3.png").arg(saveFolderPath).arg(QString::number(bedID).rightJustified(3, '0')).arg(position);
    mPrinter->bedMicroscope->save_image(fileName);
//    ui->imageDisplay->saveCurrentFrame(fileName);
}

#include "bedmicroscopewidget.moc"
#include "moc_bedmicroscopewidget.cpp"
