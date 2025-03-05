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
#include <QThread>
#include <cmath>



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
    connect(ui->printVariableTestPrintEnc, &QPushButton::clicked, this, [this]() {
        encFlag = true;  // Set the flag
        variableTestPrintPressed();  // Call the desired function
    });
    connect(ui->purgeNozzlesButton, &QPushButton::clicked, this, &MJPrintheadWidget::purgeNozzles);
    connect(ui->testNozzlesButton, &QPushButton::clicked, this, &MJPrintheadWidget::testNozzles);
    //connect(ui->verifyStartLocation, &QPushButton::clicked, this, &MJPrintheadWidget::verifyPrintStartAlignment);
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
    printComplete = false;
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

    verifyPrintStartAlignment(printStartX, printStartY);

    while (!printComplete) {
        QCoreApplication::processEvents();  // This allows the event loop to continue processing events while waiting
    }
    printComplete = false;
    GSleep(80);

    // Create program to move printer into position and complete print
    std::stringstream s;



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

    std::stringstream c_testPrint;
    c_testPrint << "GCmd," << "XQ" << "\n";
    c_testPrint << "GProgramComplete," << "\n";

    emit execute_command(c_testPrint);



}

void MJPrintheadWidget::testJetPressed()
{
    // Set printhead to correct state for printing
    double xLocation = 50;
    double yLocation = 90;
    double frequency = 1500;
    double printSpeed = 125;


    int imageWidth = 1532;
    QString fileName = "mono_logo.bmp";

    printBMPatLocationEncoder(xLocation, yLocation, frequency, printSpeed, imageWidth, fileName);


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
    // QByteArray com = command;
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

    mPrinter->mjController->outputMessage(QString("\n"));

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
        // TODO: undo my hardcoding
        double printStartX = 35 + (10 * col);                                       //!!!          35 and 70 are really just guestimates
        double printStartY = 30 + (17.6 * row);                                     //!!!       17.6 is a rough estimate of the length of the printhead

        double printFreq = frequency; // Hz
        int imageLength = width; // Number of columns to jet
        QString fileNameWFolder = "BitmapTestFolder\\" + fileName;

        if(encFlag){
            printBMPatLocationEncoder(printStartX, printStartY, printFreq, printSpeed, imageLength, fileNameWFolder);
        }
        else{
            printBMPatLocation(printStartX, printStartY, printFreq, printSpeed, imageLength, fileNameWFolder);
        }

        while (!printComplete) {
            QCoreApplication::processEvents(); // This allows the event loop to continue processing events while waiting
            // mPrinter->mjController->outputMessage(QString("Made it to while loop"));
            // emit disable_user_input();
            // ui->stopPrintingButton->setEnabled(true);
        }
        printComplete = false;
        GSleep(80);
        // mPrinter->mjController->clear_all_heads_of_data();
    }
    encFlag = false;
}

void MJPrintheadWidget::printBMPatLocation(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName){
    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;

    double accelerationSpeed = 5000;

    //double accTime = printSpeed / accelerationSpeed;
    //double accDist = (1/2) * accelerationSpeed * std::pow(accTime,2);   //give the distance to get up to speed to avoid jetting at improper speeds
    //xLocation = xLocation - accDist;
    //the above code can't be implimented until we are able to begin motion and wait a set time until we begin jetting.

    // Why is this section of the code failing on the second iteration? The program does not run and get stuck in the while loop

    //Showing Paramaters
    mPrinter->mjController->outputMessage(QString("File name: %1").arg(fileName));
    mPrinter->mjController->outputMessage(QString("Parameters: "));
    mPrinter->mjController->outputMessage(QString("Start X: %1").arg(xLocation));
    mPrinter->mjController->outputMessage(QString("Start Y: %1").arg(yLocation));
    mPrinter->mjController->outputMessage(QString("Frequency: %1").arg(frequency));
    mPrinter->mjController->outputMessage(QString("Speed: %1").arg(printSpeed));
    mPrinter->mjController->outputMessage(QString("Image Width: %1").arg(imageWidth));

    // Set print frequency and mode
    mPrinter->mjController->write_line("M 3");
    mPrinter->mjController->set_printing_frequency(frequency);

    // Set printhead to correct state for printing
    mPrinter->mjController->set_absolute_start(1);

    read_in_file(fileName);

    // Create program to move printer into position and complete print
    // Do we need to clear the s variable each time? (doesn't seem like it)
    std::stringstream s_cmd;

/*
    // Move Y axis into position
    s_cmd<< CMD::set_accleration(nonPrintAxis, 600);
    s_cmd<< CMD::set_deceleration(nonPrintAxis, 600);
    s_cmd<< CMD::set_speed(nonPrintAxis, 60);
    s_cmd<< CMD::position_absolute(nonPrintAxis, yLocation * -1);
    s_cmd<< CMD::begin_motion(nonPrintAxis);
    s_cmd<< CMD::after_motion(nonPrintAxis);

    // Move X axis_cmdinto position
    s_cmd<< CMD::set_accleration(printAxis, 600);
    s_cmd<< CMD::set_deceleration(printAxis, 600);
    s_cmd<< CMD::set_speed(printAxis, 60);
    s_cmd<< CMD::position_absolute(printAxis, xLocation);
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);
*/

    moveToLocation(xLocation, yLocation);

    while (!atLocation) {
        QCoreApplication::processEvents(); // This allows the event loop to continue processing events while waiting
        // mPrinter->mjController->outputMessage(QString("Made it to while loop"));
        // emit disable_user_input();
        // ui->stopPrintingButton->setEnabled(true);
    }
    atLocation = false;
    GSleep(80);

    double endTargetMM = xLocation + (imageWidth/frequency)*printSpeed;

    print(accelerationSpeed, printSpeed, endTargetMM, QString("Print BMP @ Location End"));
    /*
    // Start the print
    s_cmd<< CMD::set_accleration(printAxis, accelerationSpeed); // Added by Noah 9/11 -> do we need to customize the acceleration based on parameters?
    s_cmd<< CMD::set_deceleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_speed(printAxis, printSpeed);
    s_cmd<< CMD::position_absolute(printAxis, endTargetMM);
    s_cmd<< CMD::start_MJ_print();
    s_cmd<< CMD::start_MJ_dir();
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);

    // clear bits_cmdfor print start
    s_cmd<< CMD::disable_MJ_dir();
    s_cmd<< CMD::disable_MJ_start();

    s_cmd << CMD::display_message("Print Complete");                                // Added to trigger the flag

    // Compile into program for printer to run
    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // Do we need to clear the c variable every time? (doesn't seem like it)
    std::stringstream c_cmd;
    c_cmd << "GCmd," << "XQ" << "\n";
    c_cmd << "GProgramComplete," << "\n";

    emit execute_command(c_cmd);
    */
}

void MJPrintheadWidget::printBMPatLocationEncoder(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName){
    mPrinter->mjController->outputMessage(QString("Entered printBMPatLocationEncoder!!"));

    Axis printAxis = Axis::X;

    double accelerationSpeed = 5000;

    //double accTime = printSpeed / accelerationSpeed;
    //double accDist = (1/2) * accelerationSpeed * std::pow(accTime,2);   //give the distance to get up to speed to avoid jetting at improper speeds
    //xLocation = xLocation - accDist;
    //the above code can't be implimented until we are able to begin motion and wait a set time until we begin jetting.

    // Why is this section of the code failing on the second iteration? The program does not run and get stuck in the while loop

    //Showing Paramaters
    mPrinter->mjController->outputMessage(QString("File name: %1").arg(fileName));
    mPrinter->mjController->outputMessage(QString("Parameters: "));
    mPrinter->mjController->outputMessage(QString("Start X: %1").arg(xLocation));
    mPrinter->mjController->outputMessage(QString("Start Y: %1").arg(yLocation));
    mPrinter->mjController->outputMessage(QString("Frequency: %1").arg(frequency));
    mPrinter->mjController->outputMessage(QString("Speed: %1").arg(printSpeed));
    mPrinter->mjController->outputMessage(QString("Image Width: %1").arg(imageWidth));

    // Finds the necessary distance to back up the x-axis to allow for proper time and distance to reach the required velocity
    double backUpDistance = (pow(printSpeed,2.0) / (2.0 * accelerationSpeed)) * 6.0;

    // Converts the backUpDistance in "mm" to encoder steps
    double safetyVal = 1.5;
    double EncConvVal = X_CNTS_PER_MM;         //X_CNTS_PER_MM = 1000 supposedly
    double backUpdistanceEnc = backUpDistance * EncConvVal * safetyVal;

    // Adjusts the x-axis start location to allow for a speed up period
    xLocation -= backUpDistance;

    // Set print frequency and mode
    mPrinter->mjController->write_line("M 4");
    mPrinter->mjController->set_printing_frequency(frequency);

    // Begins jetting after that many encoder steps have passed
    mPrinter->mjController->set_absolute_start(backUpdistanceEnc);

    // TODO: currently how the system is set up it will zero the encoder before it begins its motion to the actual start position. fix this
    // option 1: account for that movement in the "backUpDistanceEnc" variable to include not only the backUpDistance but also any required movement to get there
    // option 2: have two separate commands that are sent one after another.
    //              command 1: move to start position and report that it did it
    //              command 2: reset values and run the set_absolute_start command then begin the print pass


    read_in_file(fileName);

    // Create program to move printer into position and complete print
    std::stringstream s_cmd;
    s_cmd.str(""); // Set the content of the stream to an empty string
    s_cmd.clear(); // Clear any error flags

    mPrinter->mjController->outputMessage(QString("XStart: " + QString::number(xLocation)));
    mPrinter->mjController->outputMessage(QString("YStart: " + QString::number(yLocation)));

    moveToLocation(xLocation, yLocation);

    while (!atLocation) {
        QCoreApplication::processEvents(); // This allows the event loop to continue processing events while waiting
        // mPrinter->mjController->outputMessage(QString("Made it to while loop"));
        // emit disable_user_input();
        // ui->stopPrintingButton->setEnabled(true);
    }
    atLocation = false;
    GSleep(80);

    /*                                                          // replaced by the moveToLocation function. done to separate the moving to start position step and actual print step for encoder counting
    // Move Y axis into position
    s_cmd<< CMD::set_accleration(nonPrintAxis, 600);
    s_cmd<< CMD::set_deceleration(nonPrintAxis, 600);
    s_cmd<< CMD::set_speed(nonPrintAxis, 60);
    s_cmd<< CMD::position_absolute(nonPrintAxis, yLocation * -1);
    s_cmd<< CMD::begin_motion(nonPrintAxis);
    s_cmd<< CMD::after_motion(nonPrintAxis);

    // Move X axis into position
    s_cmd<< CMD::set_accleration(printAxis, 600);
    s_cmd<< CMD::set_deceleration(printAxis, 600);
    s_cmd<< CMD::set_speed(printAxis, 60);
    s_cmd<< CMD::position_absolute(printAxis, xLocation);
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);
    */

    double endTargetMM = xLocation + (imageWidth/frequency)*printSpeed + (backUpDistance);
    //printEnc(accelerationSpeed, printSpeed, totalEncoderCount, QString("Printed BMP @ Location ENCODER"));

    // REPLACED BY THE PRINTENC FUNCTION.
    // Start the print
    s_cmd<< CMD::set_accleration(printAxis, accelerationSpeed); // Added by Noah 9/11 -> do we need to customize the acceleration based on parameters?
    s_cmd<< CMD::set_deceleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_speed(printAxis, printSpeed);
    s_cmd<< CMD::position_absolute(printAxis, endTargetMM);
    // s_cmd<< CMD::start_MJ_print();        // this was added to force the printer to start by sending a fake step signal to the printhead. this should be taken care of by the encoder connection
    // s_cmd<< CMD::start_MJ_dir();          // also added to take care of the start time through a fake step signal to the printhead
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);

    // clear bits_cmdfor print start
    // s_cmd<< CMD::disable_MJ_dir();       // remenant of the step input
    // s_cmd<< CMD::disable_MJ_start();     // remenant of the step input

    s_cmd << CMD::display_message("Print Complete");                                // Added to trigger the flag

    // S Value Debugging

        //CMD::string str = s.str();
        //QString sStr = QString::fromStdString(str);
        //mPrinter->mjController->outputMessage(sStr);


    // Compile into program for printer to run
    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // Do we need to clear the c variable every time? (doesn't seem like it)
    std::stringstream c_cmd;
    c_cmd << "GCmd," << "XQ" << "\n";
    c_cmd << "GProgramComplete," << "\n";

    emit execute_command(c_cmd);

}

void MJPrintheadWidget::verifyPrintStartAlignment(double xStart, double yStart){
    mPrinter->mjController->outputMessage(QString("Verifying Start Location"));


    Axis printAxis = Axis::X;
    double accelerationSpeed = 1000; // it won't actually move

    moveToLocation(xStart, yStart);

    // Set print frequency and mode
    mPrinter->mjController->write_line("M_4");
    mPrinter->mjController->set_printing_frequency(1000);

    // Set printhead to correct state for printing
    mPrinter->mjController->set_absolute_start(1);

    // Send image to printhead
    const QString fileName = "LocationVerification.bmp";
    read_in_file(fileName);

    // Create program to print
    std::stringstream s_cmd;

    //ALREADY AT LOCATION

    // Start print
    s_cmd<< CMD::set_accleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_deceleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_speed(printAxis, 100);
    s_cmd<< CMD::position_absolute(printAxis, (xStart + 2.0));
    s_cmd<< CMD::start_MJ_print();
    s_cmd<< CMD::start_MJ_dir();
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);

    // clear bits_cmdfor print start
    s_cmd<< CMD::disable_MJ_dir();
    s_cmd<< CMD::disable_MJ_start();

    //s_cmd<< CMD::message("Print Complete");
    s_cmd << CMD::display_message("Print Complete");
    s_cmd << CMD::display_message("Alignemnt Complete");
    // S Value Debugging

    // Compile into program for printer to run
    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;
}

void MJPrintheadWidget::moveToLocation(double xLocation, double yLocation){
    atLocation = false;

    mPrinter->mjController->outputMessage(QString("Entered moveToLocation!!"));
    mPrinter->mjController->outputMessage(QString("XLocation = " + QString::number(xLocation)));
    mPrinter->mjController->outputMessage(QString("YLocation = " + QString::number(yLocation)));


    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;


    std::stringstream s_cmdMove;
    s_cmdMove.str(""); // Set the content of the stream to an empty string
    s_cmdMove.clear(); // Clear any error flags

    // Move Y axis into position
    s_cmdMove<< CMD::set_accleration(nonPrintAxis, 600);
    s_cmdMove<< CMD::set_deceleration(nonPrintAxis, 600);
    s_cmdMove<< CMD::set_speed(nonPrintAxis, 60);
    s_cmdMove<< CMD::position_absolute(nonPrintAxis, yLocation * -1.0);
    s_cmdMove<< CMD::begin_motion(nonPrintAxis);
    s_cmdMove<< CMD::after_motion(nonPrintAxis);


    // Move X axis into position
    s_cmdMove<< CMD::set_accleration(printAxis, 600);
    s_cmdMove<< CMD::set_deceleration(printAxis, 600);
    s_cmdMove<< CMD::set_speed(printAxis, 60);
    s_cmdMove<< CMD::position_absolute(printAxis, xLocation);
    s_cmdMove<< CMD::begin_motion(printAxis);
    s_cmdMove<< CMD::after_motion(printAxis);


    // Verification that it has moved
    s_cmdMove << CMD::display_message("Arrived at Location");



    // Compile into program for printer to run
    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmdMove);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // Do we need to clear the c variable every time? (doesn't seem like it)
    std::stringstream c_cmdMove;
    c_cmdMove << "GCmd," << "XQ" << "\n";
    c_cmdMove << "GProgramComplete," << "\n";

    emit execute_command(c_cmdMove);
}

void MJPrintheadWidget::print(double acceleration, double speed, double endTargetMM, QString endMessage){
    Axis printAxis = Axis::X;

    // Create program to move printer into position and complete print
    // Do we need to clear the s variable each time? (doesn't seem like it)
    std::stringstream s_cmd;
    s_cmd.str(""); // Set the content of the stream to an empty string
    s_cmd.clear(); // Clear any error flags

    // Start the print
    s_cmd<< CMD::set_accleration(printAxis, acceleration);                                  // Sets acceleration
    s_cmd<< CMD::set_deceleration(printAxis, acceleration);                                 // Sets deceleration
    s_cmd<< CMD::set_speed(printAxis, speed);                                               // Sets print speed
    s_cmd<< CMD::position_absolute(printAxis, endTargetMM);                                 // Sets desiredend target in milimeters (mm)
    s_cmd<< CMD::start_MJ_print();                                                          // Tells controller to send high signal to start printing
    s_cmd<< CMD::start_MJ_dir();                                                            // Same as above
    s_cmd<< CMD::begin_motion(printAxis);                                                   // Starts the movement
    s_cmd<< CMD::after_motion(printAxis);                                                   // Ends the movement ? (I am not entirely sure)

    // clear bits for print start
    s_cmd<< CMD::disable_MJ_dir();                                                          // Remenant of the step input
    s_cmd<< CMD::disable_MJ_start();                                                        // Remenant of the step input

    //s_cmd<< CMD::message("Print Complete");
    s_cmd << CMD::display_message("Print Complete");                                        // Sends "Print Complete" message to trigger flag and signal completion
    mPrinter->mjController->outputMessage(QString("End Message %1").arg(endMessage));       // Sends any message to signal completion of the specific task


    // S Value Debugging
    /*
        CMD::string str = s.str();
        QString sStr = QString::fromStdString(str);
        mPrinter->mjController->outputMessage(sStr);
        */

    // Compile into program for printer to run
    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // Do we need to clear the c variable every time? (doesn't seem like it)
    std::stringstream c_cmd;
    c_cmd << "GCmd," << "XQ" << "\n";
    c_cmd << "GProgramComplete," << "\n";

    emit execute_command(c_cmd);
}

void MJPrintheadWidget::printEnc(double acceleration, double speed, double endTargetMM, QString endMessage){
    Axis printAxis = Axis::X;

    // Create program to move printer into position and complete print
    std::stringstream s_cmd;
    s_cmd.str(""); // Set the content of the stream to an empty string
    s_cmd.clear(); // Clear any error flags

    // Start the print
    s_cmd<< CMD::set_accleration(printAxis, acceleration);                              // Sets acceleration
    s_cmd<< CMD::set_deceleration(printAxis, acceleration);                             // Sets deceleration
    s_cmd<< CMD::set_speed(printAxis, speed);                                           // Sets print speed
    s_cmd<< CMD::position_absolute(printAxis, endTargetMM);                             // Sets desired end target in milimeters (mm)
    s_cmd<< CMD::begin_motion(printAxis);                                               // Starts the movement
    s_cmd<< CMD::after_motion(printAxis);                                               // Ends the movement ? (I am not entirely sure)

    //s_cmd<< CMD::message("Print Complete");
    s_cmd << CMD::display_message("Print Complete");                                    // Sends "Print Complete" message to trigger flag and signal completion
    mPrinter->mjController->outputMessage(QString("End Message %1").arg(endMessage));   // Sends any message to signal completion of the specific task


    // S Value Debugging
    /*
        CMD::string str = s.str();
        QString sStr = QString::fromStdString(str);
        mPrinter->mjController->outputMessage(sStr);
        */

    // Compile into program for printer to run
    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    std::stringstream c_cmd;
    c_cmd << "GCmd," << "XQ" << "\n";
    c_cmd << "GProgramComplete," << "\n";

    emit execute_command(c_cmd);
}

void MJPrintheadWidget::purgeNozzles()
{
    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;

    double accelerationSpeed = 1000;

    // Set print frequency and mode
    mPrinter->mjController->write_line("M 3");
    mPrinter->mjController->set_printing_frequency(1000);

    // Set printhead to correct state for printing
    mPrinter->mjController->set_absolute_start(1);

    // Send image to printhead (made a const 9/11)
    const QString fileNameFolder = QString("purge.bmp");         //This could be better done instead of hardcoded
    read_in_file(fileNameFolder);

    // Create program to move printer into position and complete print
    // Do we need to clear the s variable each time? (doesn't seem like it)
    std::stringstream s_cmd;
    // s.str(""); // Set the content of the stream to an empty string
    // s.clear(); // Clear any error flags

    // Move Y axis into position
    s_cmd<< CMD::set_accleration(nonPrintAxis, 600);
    s_cmd<< CMD::set_deceleration(nonPrintAxis, 600);
    s_cmd<< CMD::set_speed(nonPrintAxis, 60);
    s_cmd<< CMD::position_absolute(nonPrintAxis, 0);
    s_cmd<< CMD::begin_motion(nonPrintAxis);
    s_cmd<< CMD::after_motion(nonPrintAxis);

    // Move X axis_cmdinto position
    s_cmd<< CMD::set_accleration(printAxis, 600);
    s_cmd<< CMD::set_deceleration(printAxis, 600);
    s_cmd<< CMD::set_speed(printAxis, 60);
    s_cmd<< CMD::position_absolute(printAxis, 0);
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);

    // Start the print
    s_cmd<< CMD::set_accleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_deceleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_speed(printAxis, 100);
    s_cmd<< CMD::position_absolute(printAxis, 1);
    s_cmd<< CMD::start_MJ_print();
    s_cmd<< CMD::start_MJ_dir();
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);

    // clear bits_cmdfor print start
    s_cmd<< CMD::disable_MJ_dir();
    s_cmd<< CMD::disable_MJ_start();

    //s_cmd<< CMD::message("Print Complete");
    s_cmd << CMD::display_message("Print Complete");
    // S Value Debugging
    /*
        CMD::string str = s.str();
        QString sStr = QString::fromStdString(str);
        mPrinter->mjController->outputMessage(sStr);
        */

    // Compile into program for printer to run
    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g)
    {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // Do we need to clear the c variable every time? (doesn't seem like it)
    std::stringstream c_cmd;
    c_cmd << "GCmd," << "XQ" << "\n";
    c_cmd << "GProgramComplete," << "\n";

    emit execute_command(c_cmd);
}

void MJPrintheadWidget::testNozzles()
{
    mPrinter->mjController->outputMessage(QString("Entered Nozzle Test"));              //testing !!!!!!

    // Check if power is already on; if not, turn it on
    if (!ui->powerToggleButton->isChecked())
    {
        mPrinter->mjController->power_on();
    }

    for(int i = 0; i < 2; i++){ //fileNames.size();
        printComplete = false;
        // Set printing parameters
        QString fileName = "";
        double printSpeed = 100;
        double printStartX = 35;
        double printStartY = 200 + 20 * i;
        double printFreq = 1000; // Hz
        int imageLength = 1000; // Number of columns to jet
        if(i == 0){
            fileName = "nozzle_Test1.bmp";
        }
        else{
            fileName = "nozzle_Test2.bmp";
        }

        printBMPatLocation(printStartX, printStartY, printFreq, printSpeed, imageLength, fileName);

        while (!printComplete) {
            QCoreApplication::processEvents(); // This allows the event loop to continue processing events while waiting
            // mPrinter->mjController->outputMessage(QString("Made it to while loop"));
            // emit disable_user_input();
            // ui->stopPrintingButton->setEnabled(true);
        }
        printComplete = false;
        GSleep(80);
        // mPrinter->mjController->clear_all_heads_of_data();
    }
}


