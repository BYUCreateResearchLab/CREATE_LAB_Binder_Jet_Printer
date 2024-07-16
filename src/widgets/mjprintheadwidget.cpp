#include "mjprintheadwidget.h"
#include "ui_mjprintheadwidget.h"

#include "mjdriver.h"
#include "printer.h"
#include "mainwindow.h"
#include "dmc4080.h"

#include <QLineEdit>
#include <QDebug>

MJPrintheadWidget::MJPrintheadWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::MJPrintheadWidget)
{
    ui->setupUi(this);
    setAccessibleName("Multi-Jet Printhead Widget");

    connect(ui->connectButton, &QPushButton::clicked, this, &MJPrintheadWidget::connect_to_printhead);
    connect(ui->clearButton, &QPushButton::clicked, this, &MJPrintheadWidget::clear_response_text);
    connect(ui->inputLineEdit, &QLineEdit::returnPressed, this, &MJPrintheadWidget::command_entered);
    connect(ui->setFreqSpinBox, &QSpinBox::editingFinished, this, &MJPrintheadWidget::frequencyChanged);
    connect(ui->requestPHStatusPushButton, &QPushButton::clicked, this, [this]{mPrinter->mjController->request_status_of_all_heads();});
    connect(ui->powerToggleButton, &QPushButton::clicked, this, &MJPrintheadWidget::powerTogglePressed);
    connect(ui->getStatusButton, &QPushButton::clicked, this, [this]{mPrinter->mjController->report_status();});
    connect(ui->getPositionButton, &QPushButton::clicked, this, &MJPrintheadWidget::getPositionPressed);
    connect(ui->setVoltageSpinBox, &QSpinBox::editingFinished, this, &MJPrintheadWidget::voltageChanged);
    connect(ui->getHeadTempButton, &QPushButton::clicked, this, &MJPrintheadWidget::getHeadTempsPressed);
    connect(ui->setStartSpinBox, &QSpinBox::editingFinished, this, &MJPrintheadWidget::absoluteStartChanged);
    connect(ui->imageFileLineEdit, &QLineEdit::returnPressed, this, &MJPrintheadWidget::file_name_entered);
    connect(ui->homePrinterButton, &QPushButton::clicked, this, &MJPrintheadWidget::homePrinterPressed);
    connect(ui->setupBasicPrintButton, &QPushButton::clicked, this, &MJPrintheadWidget::setupBasicPrintPressed);
    connect(ui->startBasicPrintButton, &QPushButton::clicked, this, &MJPrintheadWidget::startBasicPrintPressed);
    connect(ui->moveToPrintPosButton, &QPushButton::clicked, this, &MJPrintheadWidget::moveToPrintPosPressed);
    connect(ui->stopPrintingButton, &QPushButton::clicked, this, &MJPrintheadWidget::stopPrintingPressed);
    connect(ui->testPrintButton, &QPushButton::clicked, this, &MJPrintheadWidget::testPrintPressed);
    connect(ui->testJetButton, &QPushButton::clicked, this, &MJPrintheadWidget::testJetPressed);

    connect(mPrinter->mjController, &AsyncSerialDevice::response, this, &MJPrintheadWidget::write_to_response_window);
}

MJPrintheadWidget::~MJPrintheadWidget()
{
    delete ui;
}

void MJPrintheadWidget::allow_widget_input(bool allowed)
{

}

void MJPrintheadWidget::connect_to_printhead()
{
    mPrinter->mjController->connect_board();
}

void MJPrintheadWidget::clear_response_text()
{
    ui->responseTextEdit->clear();
}

void MJPrintheadWidget::command_entered()
{
    QString command = ui->inputLineEdit->text();
    send_command(command);
}

void MJPrintheadWidget::file_name_entered()
{
    QString filename = ui->imageFileLineEdit->text();
    read_in_file(filename);
}

void MJPrintheadWidget::read_in_file(const QString &filename)
{
    const QString directory = "C:\\Users\\CB140LAB\\Desktop\\Noah\\";
    QString filePath = directory + filename;

    QImage image(filePath);

    if (image.isNull())
    {
        mPrinter->mjController->emit response(QString("Failed to load image from" + filePath));
        return;
    }

    mPrinter->mjController->send_image_data(1, image, 0, 2, 5);

}

void MJPrintheadWidget::powerTogglePressed()
{
    if (ui->powerToggleButton->isChecked())
        mPrinter->mjController->power_off();
    else
        mPrinter->mjController->power_on();
}

void MJPrintheadWidget::getPositionPressed()
{
    mPrinter->mjController->report_current_position();
}

void MJPrintheadWidget::getHeadTempsPressed()
{
    mPrinter->mjController->report_head_temps();
}

void MJPrintheadWidget::send_command(const QString &command)
{
//    ui->responseTextEdit->appendPlainText(command);
    ui->responseTextEdit->moveCursor(QTextCursor::End);
    ui->responseTextEdit->insertPlainText(command + "\n");
    ui->responseTextEdit->moveCursor(QTextCursor::End);
    mPrinter->mjController->write_line(command.toUtf8());
}

void MJPrintheadWidget::frequencyChanged()
{
    int freq = ui->setFreqSpinBox->value();
    mPrinter->mjController->set_printing_frequency(freq);
}

void MJPrintheadWidget::voltageChanged()
{
    double volt = ui->setVoltageSpinBox->value();
    mPrinter->mjController->set_head_voltage(Added_Scientific::Controller::HEAD1, volt);
}

void MJPrintheadWidget::absoluteStartChanged()
{
    double steps = ui->setStartSpinBox->value();
    mPrinter->mjController->set_absolute_start(steps);
}

void MJPrintheadWidget::write_to_response_window(const QString &text)
{
    ui->responseTextEdit->appendPlainText(text);
//    ui->responseTextEdit->moveCursor(QTextCursor::End);
//    ui->responseTextEdit->insertPlainText(text);
//    ui->responseTextEdit->moveCursor(QTextCursor::End);
}

void MJPrintheadWidget::homePrinterPressed()
{
// reference functions from mainwindow.cpp/printer.h to home the printer
    std::stringstream s;

    s << CMD::set_accleration(Axis::Y, 400);
    s << CMD::set_deceleration(Axis::Y, 400);
    s << CMD::set_accleration(Axis::X, 400);
    s << CMD::set_deceleration(Axis::X, 400);

    s << CMD::set_speed(Axis::Y, 60);
    s << CMD::set_speed(Axis::X, 80);

    s << CMD::position_absolute(Axis::X, 0);
    s << CMD::position_absolute(Axis::Y, 0);
    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);

    emit execute_command(s);
}

void MJPrintheadWidget::moveToPrintPosPressed()
{
    // Determine new printhead offsets compared to single nozzle
    int x_offset = 12;
    int y_offset = 12;

    // Set Print start coordinates
    int printStartX = 0;
    int printStartY = 0;

    std::stringstream s;

    s << CMD::set_accleration(Axis::Y, 400);
    s << CMD::set_deceleration(Axis::Y, 400);
    s << CMD::set_accleration(Axis::X, 400);
    s << CMD::set_deceleration(Axis::X, 400);

    s << CMD::set_speed(Axis::Y, 60);
    s << CMD::set_speed(Axis::X, 80);

    s << CMD::position_absolute(Axis::X, printStartX + x_offset);
    s << CMD::position_absolute(Axis::Y, printStartY + y_offset);
    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);

    emit execute_command(s);
}

void MJPrintheadWidget::setupBasicPrintPressed()
{
    // Put printhead into external drop watching mode ("M 2")
    mPrinter->mjController->external_dropwatch_mode();
    // Enable all nozzles to print for the first test ("I 1")
    // mPrinter->mjController->enable_all_nozzles();
    // Set Print Frequency to around 1000 Hz
    mPrinter->mjController->set_printing_frequency(1000);
}

void MJPrintheadWidget::startBasicPrintPressed()
{
    int printEndX = 4;
    int printSpeed = 80;

    // Move printer left a few cm to print lines
    std::stringstream s;

    s << CMD::set_accleration(Axis::X, 400);
    s << CMD::set_deceleration(Axis::X, 400);

    s << CMD::set_speed(Axis::X, printSpeed);
    s << CMD::position_absolute(Axis::X, printEndX);
    s << CMD::begin_motion(Axis::X);

    s << CMD::motion_complete(Axis::X);

    // Enable all nozzles and start them jetting
    mPrinter->mjController->enable_all_nozzles();

    emit execute_command(s);

    // Turn off nozzles jetting -> will this work? or will the program jump right to this as
    // commands are being executed?
    // mPrinter->mjController->clear_nozzles();

}

void MJPrintheadWidget::stopPrintingPressed()
{
    mPrinter->mjController->clear_nozzles();
}

void MJPrintheadWidget::testPrintPressed()
{
    // Set printing parameters
    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;
    int printSpeed = 60;
    int printStartX = 45;
    int printFreq = 1024; // Hz
    int imageLength = 1532; // Number of columns to jet

    // Set printhead to correct state for printing
    mPrinter->mjController->set_printing_frequency(printFreq);
    mPrinter->mjController->write_line("M 3");
    mPrinter->mjController->power_on();
    mPrinter->mjController->set_absolute_start(1);

    // Send image to printhead
    const QString filename = "mono_logo.bmp";
    read_in_file(filename);

    // Create program to move printer into position and complete print
    std::stringstream s;

    // Move Y axis into position
    s << CMD::set_accleration(nonPrintAxis, 600);
    s << CMD::set_deceleration(nonPrintAxis, 600);
    s << CMD::set_speed(nonPrintAxis, 60);
    s << CMD::position_absolute(nonPrintAxis, -80);
    s << CMD::begin_motion(nonPrintAxis);
    s << CMD::after_motion(nonPrintAxis);

    // Move X axis into position
    s << CMD::set_accleration(printAxis, 600);
    s << CMD::set_deceleration(printAxis, 600);
    s << CMD::set_speed(printAxis, 60);
    s << CMD::position_absolute(printAxis, printStartX);
    s << CMD::begin_motion(printAxis);
    s << CMD::after_motion(printAxis);

    // Start the print
    s << CMD::set_speed(printAxis, printSpeed);
    s << CMD::position_absolute(printAxis, printStartX + (imageLength/printFreq)*printSpeed);
    s << CMD::start_MJ_print(); // For some reason, the printhead isn't receiving the signal to start jetting...
    s << CMD::start_MJ_dir();
    s << CMD::begin_motion(printAxis);
    s << CMD::after_motion(printAxis);

    // Compile into program for printer to run
    std::string returnString = CMD::cmd_buf_to_dmc(s);
    const char *commands = returnString.c_str();

    qDebug().noquote() << commands;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, commands, "");
    }

    std::stringstream c;
    c << "GCmd," << "XQ" << "\n";
    c << "GProgramComplete," << "\n";
    c << CMD::disable_MJ_start();
    c << CMD::disable_MJ_dir();

    emit execute_command(c);

    mPrinter->mjController->power_off();
}

void MJPrintheadWidget::testJetPressed()
{
    // Set printhead to correct state for printing
    mPrinter->mjController->set_printing_frequency(1024);
    mPrinter->mjController->power_on();
    mPrinter->mjController->write_line("M 3");
    mPrinter->mjController->set_absolute_start(1);

    // Send image to printhead
    const QString filename = "mono_logo.bmp";
    read_in_file(filename);

    std::stringstream s;

    s << CMD::start_MJ_print();
    s << CMD::start_MJ_dir();
    emit execute_command(s);
}
