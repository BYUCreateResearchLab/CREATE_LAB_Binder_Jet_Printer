#include "mjprintheadwidget.h"
#include "ui_mjprintheadwidget.h"

#include "mjdriver.h"
#include "printer.h"
#include "mainwindow.h"
#include "dmc4080.h"
#include "outputwindow.h"

#include <QLineEdit>
#include <QDebug>
#include <sstream>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDataStream>


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
    connect(ui->stopPrintingButton, &QPushButton::clicked, this, &MJPrintheadWidget::stopPrintingPressed);
    connect(ui->testPrintButton, &QPushButton::clicked, this, &MJPrintheadWidget::testPrintPressed);
    connect(ui->testJetButton, &QPushButton::clicked, this, &MJPrintheadWidget::testJetPressed);
    connect(ui->createBitmapButton, &QPushButton::clicked, this, &MJPrintheadWidget::createBitmapPressed);
    connect(ui->singleNozzlePushButton, &QPushButton::clicked, this, &MJPrintheadWidget::singleNozzlePressed);
    connect(ui->createTestBitmaps, &QPushButton::clicked, this, &MJPrintheadWidget::createTestBitmapsPressed);
    connect(mPrinter->mjController, &AsyncSerialDevice::response, this, &MJPrintheadWidget::write_to_response_window);
    connect(ui->printVariableTestPrint, &QPushButton::clicked, this, &MJPrintheadWidget::variableTestPrintPressed);
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

    mPrinter->mjController->send_image_data(1, image, 0);

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

void MJPrintheadWidget::stopPrintingPressed()
{
    mPrinter->mjController->clear_nozzles();
}

void MJPrintheadWidget::testPrintPressed()
{
    // Set printing parameters
    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;
    int printSpeed = 100;
    int printStartX = 35;
    int printStartY = -ui->testPrintYPosSpinBox->value();
    double printFreq = 1521; // Hz
    int imageLength = 1521; // Number of columns to jet

    // Set printhead to correct state for printing
    mPrinter->mjController->set_printing_frequency(printFreq);

    // Check if power is already on; if not, turn it on
    if (!ui->powerToggleButton->isChecked())
    {
        mPrinter->mjController->power_on();
    }

    mPrinter->mjController->write_line("M 3");
    mPrinter->mjController->set_absolute_start(1);

    // Send image to printhead
    const QString filename = "currentBitmap.bmp";
    read_in_file(filename);

    // Create program to move printer into position and complete print
    std::stringstream s;

    // Move Y axis into position
    s << CMD::set_accleration(nonPrintAxis, 600);
    s << CMD::set_deceleration(nonPrintAxis, 600);
    s << CMD::set_speed(nonPrintAxis, 60);
    s << CMD::position_absolute(nonPrintAxis, printStartY);
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
    s << CMD::start_MJ_print();
    s << CMD::start_MJ_dir();
    s << CMD::begin_motion(printAxis);
    s << CMD::after_motion(printAxis);

    // clear bits for print start
    s << CMD::disable_MJ_dir();
    s << CMD::disable_MJ_start();

    s << CMD::display_message("Print Complete");

    // Compile into program for printer to run
    std::string returnString = CMD::cmd_buf_to_dmc(s);
    const char *commands = returnString.c_str();

    qDebug().noquote() << commands;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, commands, "");
    }

//    int errorCode = 0;
//        if (mPrinter->mcu->g) {
//            errorCode = GProgramDownload(mPrinter->mcu->g, commands, "");
//            if (errorCode != 0) {
//                qDebug() << "GProgramDownload error:" << errorCode;
//                return;
//            }
//        }

    std::stringstream c;
    c << "GCmd," << "XQ" << "\n";
    c << "GProgramComplete," << "\n";

    emit execute_command(c);

    // mPrinter->mjController->clear_nozzles();

//    mPrinter->mjController->power_off();
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

void MJPrintheadWidget::createBitmapPressed()
{
    int numLines = ui->numLinesSpinBox->value();
    int width = ui->pixelWidthSpinBox->value();
    mPrinter->mjController->create_bitmap_lines(numLines, width);
}

void MJPrintheadWidget::singleNozzlePressed()
{
    // Change to drop watching mode
    mPrinter->mjController->write_line("M 2");

    // Tell which nozzles to use from spin box
    QString nozzleNum = ui->nozzleNumSpinBox->text();
    QByteArray command = "n 1 ";
    command.append(nozzleNum);
    QByteArray com = command;
    mPrinter->mjController->write_line(command);
}

void MJPrintheadWidget::createTestBitmapsPressed()
{
    int numberOfLines = ui ->numberOfLines->value();
    int lineSpacing = ui ->initialYSpacing->value();
    int frequencyChange = ui ->frequencyChange->value();
    int frequency = ui->initialFrequency->value();
    int lineLength = ui->lineLength->value();
    int dropletSpacing = ui->dropletSpacing->value();
    int dropletSpacingChange = ui->dropletSpacingChange->value();

    mPrinter->mjController->createBitmapSet(numberOfLines, lineSpacing, dropletSpacing, frequency, lineLength, frequencyChange, dropletSpacingChange);
}

void MJPrintheadWidget::variableTestPrintPressed(){
    mPrinter->mjController->outputMessage(QString("Entered Variable Test Print Pressed"));              //testing !!!!!!
    // Define the directory path
    QString directoryPath = "C:\\Users\\CB140LAB\\Desktop\\Noah\\BitmapTestFolder";
    // Create a QDir object with the directory path
    QDir dir(directoryPath);
    // Get a list of all files in the directory (excluding directories)
    QStringList fileNames = dir.entryList(QDir::Files);


    QString length = QString::number(fileNames.size());
    mPrinter->mjController->outputMessage(QString("File Length: ") + length);

    // Check if power is already on; if not, turn it on
    if (!ui->powerToggleButton->isChecked())
    {
        mPrinter->mjController->power_on();
    }

    for(int i = 0; i < fileNames.size(); i++){ //fileNames.size();
        printComplete = false;
        QString fileName = fileNames[i];
        mPrinter->mjController->outputMessage(QString("\n"));                                            //testing !!!!!!
        mPrinter->mjController->outputMessage(fileName);                                            //testing !!!!!!

        //Loads Image (could also include a third argument with the folder if we want multiple folder locations)
        QString fullPath = QString("%1\\%2").arg(directoryPath, fileName);
        QImage image (fullPath);

        //Error catching for Image
        if(image.isNull()){
            mPrinter->mjController->outputMessage(QString("Failed to load image: ") + fullPath );
            continue;
        }

        //Pulling information from fileName
        QString frequencyStr = fileName.mid(fileName.length() -8, 4);
        QString speedStr = fileName.mid(fileName.length() - 14, 5);
        QString rowStr = fileName.at(0);
        QString colStr = fileName.at(1);

        bool ok;
        //Conversion from strings to Int or Double for actual use
        int frequency = frequencyStr.toInt(&ok);
        if (!ok) {
            mPrinter->mjController->outputMessage(QString("Failed to convert frequency from: ") + frequencyStr);
            continue;  // Skip to the next file
        }
        double speed = speedStr.toDouble(&ok);
        if (!ok) {
            mPrinter->mjController->outputMessage(QString("Failed to convert speed from: ") + speedStr);
            continue;  // Skip to the next file
        }
        int row = rowStr.toInt(&ok);
        if (!ok) {
            mPrinter->mjController->outputMessage(QString("Failed to convert row from: ") + rowStr);
            continue;  // Skip to the next file
        }
        int col = colStr.toInt(&ok);
        if (!ok) {
            mPrinter->mjController->outputMessage(QString("Failed to convert col from: ") + colStr);
            continue;  // Skip to the next file
        }

        int width = image.width();

        // Set printing parameters

        double printSpeed = speed;
        double printStartX = 35 + (10 * col);                                       //!!!          35 and 70 are really just guestimates
        double printStartY = 70 + (17.6 * row);                                     //!!!       17.6 is a rough estimate of the length of the printhead
        double printFreq = frequency; // Hz
        int imageLength = width; // Number of columns to jet

        printBMPatLocation(printStartX, printStartY, printFreq, printSpeed, imageLength, fileName);

        while (!printComplete) {
            QCoreApplication::processEvents(); // This allows the event loop to continue processing events while waiting
        }
        mPrinter->mjController->clear_all_heads_of_data();
    }
}

void MJPrintheadWidget::printBMPatLocation(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName){
    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;

    //Showing Paramaters
    /*
    mPrinter->mjController->outputMessage(QString("Parameters: "));
    mPrinter->mjController->outputMessage(QString("Start X: %1").arg(xLocation));
    mPrinter->mjController->outputMessage(QString("Start Y: %1").arg(yLocation));
    mPrinter->mjController->outputMessage(QString("Frequency: %1").arg(frequency));
    mPrinter->mjController->outputMessage(QString("Speed: %1").arg(printSpeed));
    mPrinter->mjController->outputMessage(QString("Image Width: %1").arg(imageWidth));
    */

    // Set print frequency
    mPrinter->mjController->set_printing_frequency(frequency);

    // Set printhead to correct state for printing
    mPrinter->mjController->write_line("M 3");
    mPrinter->mjController->set_absolute_start(1);

    // Send image to printhead
    QString fileNameWFolder = QString("BitmapTestFolder\\") + fileName;         //This could be better done instead of hardcoded
    read_in_file(fileNameWFolder);

    // Create program to move printer into position and complete print
    std::stringstream s;

    // Move Y axis into position
    s << CMD::set_accleration(nonPrintAxis, 600);
    s << CMD::set_deceleration(nonPrintAxis, 600);
    s << CMD::set_speed(nonPrintAxis, 60);
    s << CMD::position_absolute(nonPrintAxis, yLocation * -1);
    s << CMD::begin_motion(nonPrintAxis);
    s << CMD::after_motion(nonPrintAxis);

    // Move X axis into position
    s << CMD::set_accleration(printAxis, 600);
    s << CMD::set_deceleration(printAxis, 600);
    s << CMD::set_speed(printAxis, 60);
    s << CMD::position_absolute(printAxis, xLocation);
    s << CMD::begin_motion(printAxis);
    s << CMD::after_motion(printAxis);

    // Start the print
    s << CMD::set_speed(printAxis, printSpeed);
    s << CMD::position_absolute(printAxis, xLocation + (imageWidth/frequency)*printSpeed);
    s << CMD::start_MJ_print();
    s << CMD::start_MJ_dir();
    s << CMD::begin_motion(printAxis);
    s << CMD::after_motion(printAxis);

    // clear bits for print start
    s << CMD::disable_MJ_dir();
    s << CMD::disable_MJ_start();

    //s << CMD::message("Print Complete");
    s << CMD::display_message("Print Complete");
    // S Value Debugging
    /*
        CMD::string str = s.str();
        QString sStr = QString::fromStdString(str);
        mPrinter->mjController->outputMessage(sStr);
        */

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

    emit execute_command(c);
}
