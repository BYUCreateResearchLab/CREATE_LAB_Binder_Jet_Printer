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
#include <QTimer>

// Includes for STL processing 06/24    !!! TODO
#include <QFileDialog>
#include<QProcess>

// Includes for full bed printing with slicing
#include <QFileInfo>
#include <QTextStream>
#include <map>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>


MJPrintheadWidget::MJPrintheadWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::MJPrintheadWidget),
    //  New addition for STL processing 06/24 !!! TODO
    m_pythonProcess(nullptr)
{
    ui->setupUi(this);
    setAccessibleName("Multi-Jet Printhead Widget");

    allow_widget_input_MJ(false);

    // timer setuip
    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(100);
    connect(m_positionTimer, &QTimer::timeout, this, &MJPrintheadWidget::requestEncoderPosition);


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
    connect(ui->zeroEncoder, &QPushButton::clicked, this, &MJPrintheadWidget::zeroEncoder);
    connect(ui->startStopDisplay, &QPushButton::clicked, this, &MJPrintheadWidget::onStartStopDisplayClicked);
    connect(ui->moveNozzle, &QPushButton::clicked, this, &MJPrintheadWidget::moveNozzleOffPlate);

    // 06/24 !!! TODO
    connect(ui->sliceStlButton, &QPushButton::clicked, this, &MJPrintheadWidget::sliceStlButton_clicked);

    // connect jog buttons
    connect(ui->xRightButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::x_right_button_pressed_MJ);
    connect(ui->xLeftButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::x_left_button_pressed_MJ);
    connect(ui->yUpButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::y_up_button_pressed_MJ);
    connect(ui->yDownButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::y_down_button_pressed_MJ);

    connect(ui->xRightButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);
    connect(ui->xLeftButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);
    connect(ui->yUpButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);
    connect(ui->yDownButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);

    // connect homing buttons
    connect(ui->xHomeMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_xHome_clicked_MJ);
    connect(ui->yHomeMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_yHome_clicked_MJ);

    // connect z movement buttons
    connect(ui->zUpMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zUp_clicked_MJ);
    connect(ui->zDownMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zDown_clicked_MJ);
    connect(ui->zMaxMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zMax_clicked_MJ);
    connect(ui->zMinMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zMin_clicked_MJ);

    // connect position getter buttons
    connect(ui->getXAxisPositionMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::get_current_x_axis_position_MJ);
    connect(ui->getYAxisPositionMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::get_current_y_axis_position_MJ);
    connect(ui->getZAxisPositionMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::get_current_z_axis_position_MJ);
    connect(ui->zAbsoluteMoveButtonMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::move_z_to_absolute_position_MJ);

    // connect for printing full bed prints
    connect(ui->startFullPrintButton, &QPushButton::clicked, this, &MJPrintheadWidget::on_startFullPrintButton_clicked);


}


MJPrintheadWidget::~MJPrintheadWidget()
{
    m_positionTimer->stop();
    delete ui;
}

void MJPrintheadWidget::allow_widget_input(bool allowed)
{
    // Jogging Buttons
    ui->xRightButtonMJ->setEnabled(allowed);
    ui->xLeftButtonMJ->setEnabled(allowed);
    ui->yUpButtonMJ->setEnabled(allowed);
    ui->yDownButtonMJ->setEnabled(allowed);

    // Homing Buttons
    ui->xHomeMJ->setEnabled(allowed);
    ui->yHomeMJ->setEnabled(allowed);

    // Z-Axis Buttons
    ui->zUpMJ->setEnabled(allowed);
    ui->zDownMJ->setEnabled(allowed);
    ui->zMaxMJ->setEnabled(allowed);
    ui->zMinMJ->setEnabled(allowed);
    ui->zAbsoluteMoveButtonMJ->setEnabled(allowed);

    // Position Getters
    ui->getXAxisPositionMJ->setEnabled(allowed);
    ui->getYAxisPositionMJ->setEnabled(allowed);
    ui->getZAxisPositionMJ->setEnabled(allowed);

    // Input Spin Boxes
    ui->xVelocityMJ->setEnabled(allowed);
    ui->yVelocityMJ->setEnabled(allowed);
    ui->zStepSizeMJ->setEnabled(allowed);
    ui->zAbsoluteMoveSpinBoxMJ->setEnabled(allowed);

    // Test Movdes
    ui->purgeNozzlesButton->setEnabled(allowed);
    ui->testNozzlesButton->setEnabled(allowed);
    ui->singleNozzlePushButton->setEnabled(allowed);
    ui->startStopDisplay->setEnabled(allowed);
    ui->zeroEncoder->setEnabled(allowed);
    ui->printVariableTestPrint->setEnabled(allowed);
    ui->printVariableTestPrintEnc->setEnabled(allowed);
    ui->moveNozzle->setEnabled(allowed);
    ui->verifyStartLocation->setEnabled(allowed);
    ui->testPrintButton->setEnabled(allowed);
    ui->testJetButton->setEnabled(allowed);
    ui->startFullPrintButton->setEnabled(allowed);
}

void MJPrintheadWidget::allow_widget_input_MJ(bool allowed){
    ui->getHeadTempButton->setEnabled(allowed);
    ui->getStatusButton->setEnabled(allowed);
    ui->powerToggleButton->setEnabled(allowed);
    ui->requestPHStatusPushButton->setEnabled(allowed);
    ui->getPositionButton->setEnabled(allowed);
    ui->setVoltageSpinBox->setEnabled(allowed);
    ui->setFreqSpinBox->setEnabled(allowed);
    ui->imageFileLineEdit->setEnabled(allowed);
    ui->setStartSpinBox->setEnabled(allowed);
    ui->stopPrintingButton->setEnabled(allowed);
}

void MJPrintheadWidget::connect_to_printhead()
{
    // This function now toggles the connection to the board.
    if (mPrinter->mjController->is_connected())
    {
        allow_widget_input_MJ(true);
    }
    else
    {
        // If not connected, attempt to connect.
        mPrinter->mjController->connect_board();
        allow_widget_input_MJ(true);
    }
}

void MJPrintheadWidget::clear_response_text()
{
    ui->responseTextEdit->clear();
}

void MJPrintheadWidget::zeroEncoder()
{
    mPrinter->mjController->set_absolute_start(1);
}

void MJPrintheadWidget::requestEncoderPosition()
{
    if(mPrinter->mjController->is_connected()){
        mPrinter->mjController->report_current_position();
    }
}

void MJPrintheadWidget::onStartStopDisplayClicked()
{
    // Check if the timer is currently active
    if (m_positionTimer->isActive())
    {
        // If it is active, stop it.
        m_positionTimer->stop();
        // Update the button text to show the next available action.
        ui->startStopDisplay->setText("Start Encoder Display");
    }
    else
    {
        // If it is not active, first check if the controller is connected.
        if (mPrinter->mjController->is_connected())
        {
            // If connected, start the timer.
            m_positionTimer->start();
            // Update the button text to show the next available action.
            ui->startStopDisplay->setText("Stop Encoder Display");
        }
        else
        {
            mPrinter->mjController->outputMessage("Error: Controller is not connected.");
        }
    }
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

void MJPrintheadWidget::read_in_file(const QString &fileNameOrPath)
{
    QString filePath = fileNameOrPath;
    QFileInfo fileInfo(filePath);

    // If the path is not absolute, prepend the default directory.
    // This maintains backward compatibility for test functions.
    if (!fileInfo.isAbsolute()) {
        const QString directory = "C:\\Users\\CB140LAB\\Desktop\\Noah\\";
        filePath = directory + fileNameOrPath;
    }

    QImage image(filePath);

    if (image.isNull())
    {
        mPrinter->mjController->emit response(QString("Failed to load image from " + filePath));
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
    const QString positionPrefix = "Encoder current count: ";

    if (text.startsWith(positionPrefix)){

        QString numberOnly = text.mid(positionPrefix.length());

        m_encoderHistory.append(numberOnly);

        while(m_encoderHistory.size() > 10)
        {
            m_encoderHistory.removeFirst();
        }
        ui->responseTextEdit_2->setPlainText(m_encoderHistory.join(""));
    }
    else{
        ui->responseTextEdit->appendPlainText(text);
    }
//    ui->responseTextEdit->moveCursor(QTextCursor::End);
//    ui->responseTextEdit->insertPlainText(text);
//    ui->responseTextEdit->moveCursor(QTextCursor::End);
}

void MJPrintheadWidget::stopPrintingPressed()
{
    mPrinter->mjController->clear_nozzles();
}

void MJPrintheadWidget::moveNozzleOffPlate()
{
    moveToLocation(5, 5, QString("Moved nozzle off of build plate"));
}

void MJPrintheadWidget::testPrintPressed()
{
    // --- 1. Set Printing Parameters ---
    double printSpeed = 10;
    double truePrintStartX = 65;
    double printStartY = -65;
    double printFreq = 1000; // Hz
    double imageLength = 1000; // Number of columns to jet
    double accelerationSpeed = 1000;       // CHANGE THIS: it is low so that we can see the backup distance

    // --- 2.0 Calculate Distances & Coordinates
    double safetyVal = 300.0;     // TODO: this value will usually be aroun 2 but for testing it is increased
    double backUpDistance = (pow(printSpeed,2.0) / (2.0 * accelerationSpeed)) * safetyVal;
    double backUpDistanceEnc = backUpDistance * X_CNTS_PER_MM;
    double backedUpStartX = truePrintStartX - backUpDistance;

    // The final X-Coordinate after accelerating, printing, and decelerating
    double printDistance = (imageLength / printFreq) * printSpeed;
    double endTargetMM = backedUpStartX + backUpDistance + printDistance + backUpDistance;


    // Log calcaulted values for debugging
    mPrinter->mjController->outputMessage(QString("--- Test Print Initiated ---"));
    mPrinter->mjController->outputMessage(QString("True Start (mm): %1").arg(truePrintStartX));
    mPrinter->mjController->outputMessage(QString("Backed-up Start (mm): %1").arg(backedUpStartX));
    mPrinter->mjController->outputMessage(QString("End Target (mm): %1").arg(endTargetMM));
    mPrinter->mjController->outputMessage(QString("Backup Distance (mm): %1").arg(backUpDistance));
    mPrinter->mjController->outputMessage(QString("Trigger Distance (counts): %1").arg(backUpDistanceEnc));
    mPrinter->mjController->outputMessage(QString(""));

    // --- 3. Go to the TRUE Start Location & Print a Verification Line
    verifyPrintStartAlignment(truePrintStartX, printStartY);
    GSleep(2000);   //TODO: this is left to ensure that the print finished before moving on

    // Move from the true start to the backed-up position
    mPrinter->mjController->outputMessage(QString("Moving back to the acceleration start"));
    moveToLocation(backedUpStartX, printStartY, QString("Moved to Backed-Up Start"));

    // Wait until the head is in the backed-up position before proceeding
    while( !atLocation){
        QCoreApplication::processEvents();
    }
    atLocation = false;
    GSleep(100);

    // --- 4. Prepare & Execute the Main Print Job
    mPrinter->mjController->outputMessage(QString("Starting main print job..."));

    // Set print frequency & mode
    mPrinter->mjController->write_line("M 4");
    mPrinter->mjController->set_printing_frequency(printFreq);

    // Send main image to printhead
    const QString filename = "currentBitmap.bmp";
    read_in_file(filename);

    // Set the controller to begin jetting after moving the required encoder steps
    GSleep(100);
    mPrinter->mjController->set_absolute_start(backUpDistanceEnc);
    mPrinter->mjController->outputMessage(QString("backUpDistanceEnc: %1").arg(backUpDistanceEnc));
    mPrinter->mjController->report_current_position();  //  !!!TODO: investigate the use of this

    // BEGINS THE PRINT PROCESS from the backed-up location
    printComplete = false;
    printEnc(accelerationSpeed, printSpeed, endTargetMM, QString("Test Print Main Motion Complete"));

    while (!printComplete) {
        mPrinter->mjController->report_current_position();  //  !!!TODO: investigate the use of this 6/6
        GSleep(100);    // !!!TODO: is this needed with the print function thing 6/6
        QCoreApplication::processEvents();  // This allows the event loop to continue processing events while waiting
    }
    printComplete = false;
    GSleep(80);

    mPrinter->mjController->outputMessage(QString("--- Test Print Finished ---"));

    moveNozzleOffPlate();
}

void MJPrintheadWidget::testJetPressed()
{
    // Set printhead to correct state for printing
    double xLocation = 50;
    double yLocation = 90;
    double frequency = 1500;
    double printSpeed = 10;        // TESTING: this used to be 125


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
        double printStartY = 30 + (17.6 * row) * -1.0;                                     //!!!       17.6 is a rough estimate of the length of the printhead

        double printFreq = frequency; // Hz
        int imageLength = width; // Number of columns to jet
        QString fileNameWFolder = "BitmapTestFolder\\" + fileName;

        atLocation = false;

        if(encFlag){
            printBMPatLocationEncoder(printStartX, printStartY, printFreq, printSpeed, imageLength, fileNameWFolder);
        }
        else{
            printBMPatLocation(printStartX, printStartY, printFreq, printSpeed, imageLength, fileNameWFolder);
        }

        GSleep(100);

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

    // Finds the target end position in milimeters (mm)
    double endTargetMM = xLocation + (imageWidth/frequency)*printSpeed;

    // MOVES TO THE START LOCATION
    moveToLocation(xLocation, yLocation, QString("Print BMP Start Location"));

    // Waits to arrive at the start location
    while (!atLocation) {
        QCoreApplication::processEvents(); // This allows the event loop to continue processing events while waiting
        // mPrinter->mjController->outputMessage(QString("Made it to while loop"));
        // emit disable_user_input();
        // ui->stopPrintingButton->setEnabled(true);
    }
    atLocation = false;
    GSleep(80);

    // BEGINS THE PRINT PROCESS
    print(accelerationSpeed, printSpeed, endTargetMM, QString("Print BMP @ Location End"));

    while (!printComplete) {
        QCoreApplication::processEvents();  // This allows the event loop to continue processing events while waiting
    }
    printComplete = false;
    GSleep(80);

}

void MJPrintheadWidget::printBMPatLocationEncoder(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName){
    mPrinter->mjController->outputMessage(QString("Entered printBMPatLocationEncoder!!"));

    // --- 1. Motion & Print Parameters ---
    Axis printAxis = Axis::X;
    double accelerationSpeed = 3000;
    double safetyFactor = 2.0;      // typically probably 2 but
    double encoderCountsPerMM  = X_CNTS_PER_MM;

    // --- 2. Calculate Distances & Coordinates ---
    // Calcaulte distance needed to accelerate to target print speed
    double backUpDistance = (pow(printSpeed,2.0) / (2.0 * accelerationSpeed)) * safetyFactor;

    // Calculate the dsitance th eprinthead will travel while jetting
    double printDistance = (static_cast<double>(imageWidth) / frequency) * printSpeed;

    // Convert the backUpDistance from mm to encoder steps for the trigger
    double backUpDistanceEnc = backUpDistance * encoderCountsPerMM;

    // Determine the start and end poitns of the physical motion
    double trueStartX = xLocation;
    double backedUpStartX = trueStartX - backUpDistance;

    // Determine final target position with room for acceleration, printing, and decelleration
    double endTargetMM = backedUpStartX + backUpDistance + printDistance + backUpDistance;

    // --- 3. Log Calculated values for Debugging ---

    //Showing Paramaters
    mPrinter->mjController->outputMessage(QString("File name: %1").arg(fileName));
    mPrinter->mjController->outputMessage(QString("Parameters: X=%1, Y=%2, Freq=%3, Speed=%4").arg(xLocation).arg(yLocation).arg(frequency).arg(printSpeed));
    mPrinter->mjController->outputMessage(QString("Calculated True Start (mm): %1").arg(trueStartX));
    mPrinter->mjController->outputMessage(QString("Calculated Backed-up Start (mm): %1").arg(backedUpStartX));
    mPrinter->mjController->outputMessage(QString("Calculated End Target (mm): %1").arg(endTargetMM));
    mPrinter->mjController->outputMessage(QString("Calculated Trigger Distance (encoder counts): %1").arg(backUpDistanceEnc));

    // --- 4. Configure Printhead and Load Data ---
    mPrinter->mjController->write_line("M 4");
    mPrinter->mjController->set_printing_frequency(frequency);
    read_in_file(fileName);

    // --- 5. Move to Backed-Up Starting Position
    moveToLocation(backedUpStartX, yLocation, QString("move to Encoder Start Complete"));

    // Wait until the movement is physically complete
    while (!atLocation) {
        QCoreApplication::processEvents();
    }
    atLocation = false; // Reset flag for the next operation
    GSleep(100);

    // --- 6. Set Encoder Trigger and Execute Print ---
    // Tell the controller to start jetting after moving 'backUpDistanceEnc' steps from the start
    mPrinter->mjController->set_absolute_start(backUpDistanceEnc);

    printComplete = false;  // Reset the print completion flag

    // Begin the print motion from 'backUpStartX' to 'endTargetMM'
    printEnc(accelerationSpeed, printSpeed, endTargetMM, QString("Encoder Print Motion Complete"));

    while (!printComplete) {
        QCoreApplication::processEvents();
    }
    printComplete = false;  // Reset flag for the next operation
    GSleep(100);

    mPrinter->mjController->outputMessage(QString("--- Encoder Print Finished ---"));

}

void MJPrintheadWidget::verifyPrintStartAlignment(double xStart, double yStart){
    mPrinter->mjController->outputMessage(QString("verifyPrintStartAlignment()"));

    double accelerationSpeed = 1000; // it won't actually move

    // MOVES TO THE START LOCATION
    moveToLocation(xStart, yStart, QString("Test Print True Location"));

    // Waits to arrive at the start location
    while (!atLocation) {
        QCoreApplication::processEvents(); // This allows the event loop to continue processing events while waiting
        // mPrinter->mjController->outputMessage(QString("Made it to while loop"));
        // emit disable_user_input();
        // ui->stopPrintingButton->setEnabled(true);
    }
    atLocation = false;
    GSleep(80);

    mPrinter->mjController->outputMessage(QString("verifyPrintStartAlignment: post arrival FLAG"));


    mPrinter->mjController->write_line("M 4");
    mPrinter->mjController->set_printing_frequency(1000);

    // Set printhead to print after 1 encoder tick
    mPrinter->mjController->set_absolute_start(1);

    // Send image to printhead
    const QString fileName = "LocationVerification.bmp";
    read_in_file(fileName);

    // Create program to print
    std::stringstream s_cmd;

    printEnc(accelerationSpeed, 10, xStart + 1.0, QString("Alignment Complete"));

    while (!printComplete) {
        QCoreApplication::processEvents();  // This allows the event loop to continue processing events while waiting
    }
    printComplete = false;
    GSleep(80);

}

void MJPrintheadWidget::moveToLocation(double xLocation, double yLocation, QString endMessage){
    atLocation = false;

    mPrinter->mjController->outputMessage(QString("moveToLocation()"));
    mPrinter->mjController->outputMessage(QString("XLocation = " + QString::number(xLocation)));
    mPrinter->mjController->outputMessage(QString("YLocation = " + QString::number(yLocation)));


    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;

    std::stringstream s_cmdMove;
    s_cmdMove.str(""); // Set the content of the stream to an empty string
    s_cmdMove.clear(); // Clear any error flags

    // Move Y axis into position
    s_cmdMove<< CMD::set_accleration(nonPrintAxis, 800);
    s_cmdMove<< CMD::set_deceleration(nonPrintAxis, 800);
    s_cmdMove<< CMD::set_speed(nonPrintAxis, 60);
    s_cmdMove<< CMD::position_absolute(nonPrintAxis, yLocation);
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
    mPrinter->mjController->outputMessage(QString("End Message %1").arg(endMessage));       // Sends any message to signal completion of the specific task

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
    printComplete = false;
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

    mPrinter->mjController->outputMessage(QString("inside printEnc()"));

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
    // --- 1. Define purge parameters ---
    double truePurgeStartX = 10.0;
    double purgeY = 10.0;
    double purgeDistance = 5.0;
    double purgeSpeed = 10.0;
    double acceleration = 1000.0;
    double purgeFreq = 1000.0;
    QString purgeBitmap = "purge.bmp";

    mPrinter->mjController->outputMessage(QString("--- Encoder Purge Sequence Initiated ---"));

    // Verify power is on
    if (!ui->powerToggleButton->isChecked())
    {
        mPrinter->mjController->power_on();
        GSleep(100);
    }

    // --- 2. Calculate the Distances & Coordinates (we will ignore acceleration ramp up) ---
    double backUpDistance = 1.0;
    double backUpDistanceEnc = backUpDistance * X_CNTS_PER_MM;
    double backedUpStartX = truePurgeStartX - backUpDistance;

    // Calcaulte the final target position for the entire motion
    double endTargetMM = backedUpStartX + backUpDistance + purgeDistance + backUpDistance;

    // --- 3. Configure the Printhead for Encoder-Based Printing ---
    mPrinter->mjController->write_line("M 4");
    mPrinter->mjController->set_printing_frequency(purgeFreq);

    // Load in the purge bitmap
    read_in_file(purgeBitmap);

    // Log the calculated values for debugging
    /*
    mPrinter->mjController->outputMessage(QString("True Start (mm): %1").arg(truePurgeStartX));
    mPrinter->mjController->outputMessage(QString("Backed-up Start (mm): %1").arg(backedUpStartX));
    mPrinter->mjController->outputMessage(QString("End Target (mm): %1").arg(endTargetMM));
    mPrinter->mjController->outputMessage(QString("Trigger Distance (counts): %1").arg(backUpDistanceEnc));
    */

    // --- 4. Move to the backed-up starting position
    mPrinter->mjController->outputMessage(QString("Moving to purge acceleration start location..."));
    moveToLocation(backedUpStartX, purgeY, "Arrived at Backed-Up Purge Location");

    // Wait until the move is complete
    while (!atLocation) {
        QCoreApplication::processEvents();
    }
    atLocation = false; // Reset the flag for the next action

    // --- 5. Set the encoder trigger and execute the purge ---
    // Tell the controller to start jetting after moving 'backUpDistanceEnc' steps
    mPrinter->mjController->set_absolute_start(1);

    mPrinter->mjController->outputMessage(QString("Executing encoder-based purge..."));

    printComplete = false; // Reset the flag
    // Use the ENCODER print helper function
    printEnc(acceleration, purgeSpeed, endTargetMM, "Encoder Purge Motion Complete");

    // Wait until the purge print is complete
    while (!printComplete) {
        QCoreApplication::processEvents();
    }
    printComplete = false; // Reset the flag
    GSleep(80);

    mPrinter->mjController->outputMessage(QString("--- Encoder Purge Sequence Finished ---"));

    /*
    Axis nonPrintAxis = Axis::Y;
    Axis printAxis = Axis::X;

    double accelerationSpeed = 1000;

    // Set print frequency and mode
    mPrinter->mjController->write_line("M 4");
    mPrinter->mjController->set_printing_frequency(1000);

    // Set printhead to correct state for printing
    mPrinter->mjController->set_absolute_start(1);

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
    s_cmd<< CMD::position_absolute(nonPrintAxis, 10);
    s_cmd<< CMD::begin_motion(nonPrintAxis);
    s_cmd<< CMD::after_motion(nonPrintAxis);

    // Move X axis_cmdinto position
    s_cmd<< CMD::set_accleration(printAxis, 600);
    s_cmd<< CMD::set_deceleration(printAxis, 600);
    s_cmd<< CMD::set_speed(printAxis, 60);
    s_cmd<< CMD::position_absolute(printAxis, 10);
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);

    // Start the print
    s_cmd<< CMD::set_accleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_deceleration(printAxis, accelerationSpeed);
    s_cmd<< CMD::set_speed(printAxis, 100);
    s_cmd<< CMD::position_absolute(printAxis, 11);
    s_cmd<< CMD::start_MJ_print();
    s_cmd<< CMD::start_MJ_dir();
    s_cmd<< CMD::begin_motion(printAxis);
    s_cmd<< CMD::after_motion(printAxis);

    // clear bits_cmdfor print start
    s_cmd<< CMD::disable_MJ_dir();
    s_cmd<< CMD::disable_MJ_start();

    //s_cmd<< CMD::message("Print Complete");
    s_cmd << CMD::display_message("Print Complete");


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

QString MJPrintheadWidget::verifyPrintStartStop(int xStart, int xStop){
    int xLowerLimit = 0;
    int xUpperLimit = 150;

    if(xStart < xLowerLimit)
    {
        return QString("ERROR: Print start is outside the build plate");
    }
    else if(xStop > xUpperLimit)
    {
        return QString("ERROR: Print stop is outside the build plate");
    }
    else
    {
        return QString("Print fits inside the build plate");
    }
}

void MJPrintheadWidget::sliceStlButton_clicked(){
    mPrinter->mjController->outputMessage(QString("Initiating STL slicing... "));

    // --- 1. On button press, prompt to browse for an STL file ---
    QString stlFilePath = QFileDialog::getOpenFileName(this, tr("Open STL File"),
                                                       "C:\\Users\\CB140LAB\\Desktop\\Noah\\ComplexMultiNozzle\\STL_Files",
                                                       tr("STL Files (*.stl *.STL)"));

    if (stlFilePath.isEmpty()){
        mPrinter->mjController->outputMessage(QString("No STL file selected."));
        return;
    }

    // --- 2. Locate relavent files ---
    // Python Script Location !!!
    QString pythonScriptPath = "C:\\Users\\CB140LAB\\Desktop\\Noah\\ComplexMultiNozzle\\Python_Code\\STL_Slicing_Viewer_mod.py";

    // Path to python.exe
    QString pythonExecutable = "C:\\Users\\CB140LAB\\AppData\\Local\\Microsoft\\WindowsApps\\python3.11.exe";

    // --- 3. read values from UI text boxes ---
    int printFreqSTL = ui->printFrequencySTL->value();
    double dropletSpacingSTLum = ui->dropletSpacing->value();
    double lineSpacingSTLum = ui->lineSpacingSTL->value();
    double layerHeightSTLum = ui->layerHeightSTL->value();

    double dropletSpacingSTLmm = dropletSpacingSTLum / 1000.0;
    double lineSpacingSTLmm = lineSpacingSTLum / 1000.0;
    double layerHeightSTLmm = layerHeightSTLum / 1000.0;

    if (m_pythonProcess && m_pythonProcess->state() == QProcess::Running){
        mPrinter->mjController->outputMessage(QString("Slicer script is already running."));
        return;
    }

    m_pythonProcess = new QProcess(this);

    // Connect signals to slots to get feedback from script
    connect(m_pythonProcess, &QProcess::readyReadStandardOutput, this, &MJPrintheadWidget::readPythonOutput);
    connect(m_pythonProcess, &QProcess::readyReadStandardError, this, &MJPrintheadWidget::handlePythonError);
    connect(m_pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MJPrintheadWidget::onPythonScriptFinished);

    // List of arguments to pass to the script
    QStringList args;
    args << pythonScriptPath
         << stlFilePath
         << QString::number(printFreqSTL)
         << QString::number(dropletSpacingSTLmm)
         << QString::number(lineSpacingSTLmm)
         << QString::number(layerHeightSTLmm);

    mPrinter->mjController->outputMessage(QString("Starting Python slicer..."));
    mPrinter->mjController->outputMessage(QString("Command: %1 %2").arg(pythonExecutable, args.join(" ")));

    // --- 4. Launch the Script
    m_pythonProcess->start(pythonExecutable, args);

}

void MJPrintheadWidget::readPythonOutput()
{
    // Your implementation here. For example:
    QByteArray output = m_pythonProcess->readAllStandardOutput();
    qDebug() << "Python output:" << output;
}

void MJPrintheadWidget::handlePythonError()
{
    // Your implementation here. For example:
    QByteArray errorOutput = m_pythonProcess->readAllStandardError();
    qWarning() << "Python error:" << errorOutput;
}

void MJPrintheadWidget::onPythonScriptFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Your implementation here. For example:
    qDebug() << "Python script finished with exit code:" << exitCode;
    if (exitStatus == QProcess::CrashExit) {
        qWarning() << "The Python script crashed.";
    }
}


// MOTION ADDITION:
void MJPrintheadWidget::x_right_button_pressed_MJ()
{
    std::stringstream s;
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::set_jog(Axis::X, ui->xVelocityMJ->value());
    s << CMD::begin_motion(Axis::X);
    emit execute_command(s);
}

void MJPrintheadWidget::x_left_button_pressed_MJ()
{
    std::stringstream s;
    s << CMD::set_accleration(Axis::X, 800);
    s << CMD::set_deceleration(Axis::X, 800);
    s << CMD::set_jog(Axis::X, -ui->xVelocityMJ->value());
    s << CMD::begin_motion(Axis::X);
    emit execute_command(s);
}

void MJPrintheadWidget::y_up_button_pressed_MJ()
{
    std::stringstream s;
    s << CMD::set_accleration(Axis::Y, 300);
    s << CMD::set_deceleration(Axis::Y, 300);
    s << CMD::set_jog(Axis::Y, -ui->yVelocityMJ->value());
    s << CMD::begin_motion(Axis::Y);
    emit execute_command(s);
}

void MJPrintheadWidget::y_down_button_pressed_MJ()
{
    std::stringstream s;
    s << CMD::set_accleration(Axis::Y, 300);
    s << CMD::set_deceleration(Axis::Y, 300);
    s << CMD::set_jog(Axis::Y, ui->yVelocityMJ->value());
    s << CMD::begin_motion(Axis::Y);
    emit execute_command(s);
}

void MJPrintheadWidget::jog_released_MJ()
{
    std::stringstream s;
    s << CMD::stop_motion(Axis::X);
    s << CMD::stop_motion(Axis::Y);
    s << CMD::stop_motion(Axis::Z);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);
    emit execute_command(s);
}

void MJPrintheadWidget::on_xHome_clicked_MJ()
{
    std::stringstream s;
    Axis x{Axis::X};
    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::position_absolute(x, 0);
    s << CMD::set_speed(x, 50);
    s << CMD::begin_motion(x);
    s << CMD::motion_complete(x);
    emit execute_command(s);
}

void MJPrintheadWidget::on_yHome_clicked_MJ()
{
    std::stringstream s;
    Axis y{Axis::Y};
    s << CMD::set_accleration(y, 300);
    s << CMD::set_deceleration(y, 300);
    s << CMD::position_absolute(y, 0);
    s << CMD::set_speed(y, 50);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);
    emit execute_command(s);
}

void MJPrintheadWidget::on_zUp_clicked_MJ()
{
    std::stringstream s;
    Axis z{Axis::Z};
    double stepValue_microns = ui->zStepSizeMJ->value();
    double stepValue_mm = stepValue_microns / 1000.0;
    s << CMD::position_relative(z, stepValue_mm);
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);
    emit execute_command(s);
}

void MJPrintheadWidget::on_zDown_clicked_MJ()
{
    std::stringstream s;
    Axis z{Axis::Z};
    double stepValue_microns = ui->zStepSizeMJ->value();
    double stepValue_mm = stepValue_microns / 1000.0;
    s << CMD::position_relative(z, -stepValue_mm);
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);
    emit execute_command(s);
}

void MJPrintheadWidget::on_zMax_clicked_MJ()
{
    std::stringstream s;
    Axis z{Axis::Z};
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_jog(z, 1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);
    emit execute_command(s);
}

void MJPrintheadWidget::on_zMin_clicked_MJ()
{
    std::stringstream s;
    Axis z{Axis::Z};
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_jog(z, -1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);
    emit execute_command(s);
}

void MJPrintheadWidget::get_current_x_axis_position_MJ()
{
    if (mPrinter->mcu->g)
    {
        int currentXPos;
        GCmdI(mPrinter->mcu->g, "TPX", &currentXPos);
        double currentXPos_mm = currentXPos / (double)X_CNTS_PER_MM;
        emit print_to_output_window("Current X: " + QString::number(currentXPos_mm) + "mm");
    }
}

void MJPrintheadWidget::get_current_y_axis_position_MJ()
{
    if (mPrinter->mcu->g)
    {
        int currentYPos;
        GCmdI(mPrinter->mcu->g, "TPY", &currentYPos);
        double currentYPos_mm = currentYPos / (double)Y_CNTS_PER_MM;
        emit print_to_output_window("Current Y: " + QString::number(currentYPos_mm) + "mm");
    }
}

void MJPrintheadWidget::get_current_z_axis_position_MJ()
{
    if (mPrinter->mcu->g)
    {
        int currentZPos;
        GCmdI(mPrinter->mcu->g, "TPZ", &currentZPos);
        double currentZPos_mm = currentZPos / (double)Z_CNTS_PER_MM;
        emit print_to_output_window("Current Z: " + QString::number(currentZPos_mm) + "mm");
    }
}

void MJPrintheadWidget::move_z_to_absolute_position_MJ()
{
    std::stringstream s;
    s << CMD::position_absolute(Axis::Z, ui->zAbsoluteMoveSpinBoxMJ->value());
    s << CMD::set_accleration(Axis::Z, 10);
    s << CMD::set_deceleration(Axis::Z, 10);
    s << CMD::set_speed(Axis::Z, 1.5);
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);
    emit execute_command(s);
}

// FULL BED PRINTING 06/30
// Find the job folder.
void MJPrintheadWidget::on_startFullPrintButton_clicked()
{
    QString jobFolderPath = QFileDialog::getExistingDirectory(this, tr("Select Print Job Folder"),
                                                              "C:\\Users\\CB140LAB\\Desktop\\Noah\\ComplexMultiNozzle\\Slicing",
                                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (jobFolderPath.isEmpty()) {
        mPrinter->mjController->outputMessage("No print job folder selected. Aborting.");
        return;
    }

    // Start the print job using the selected folder
    startFullPrintJob(jobFolderPath);
}

// parses the print_parameters.txt file for important print settings
bool MJPrintheadWidget::parsePrintParameters(const QString& filePath, PrintParameters& params) {
    // NOTE: Ensure your PrintParameters struct in the .h file is updated to include:
    // QString sourceSTLFile;
    // double layerHeight = 0.0;
    // bool yShiftEnabled = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mPrinter->mjController->outputMessage(QString("ERROR: Could not open parameter file: %1").arg(filePath));
        return false;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.contains(':')) {
            QStringList parts = line.split(':');
            if (parts.size() < 2) continue;

            QString key = parts[0].trimmed();
            QString valuePart = parts.last().trimmed();

            if (key == "Source STL File") {
                params.fileName = valuePart;
            }
            else if (key == "Layer Height (Z)") {
                params.layerHeight = valuePart.split(' ')[0].toDouble();
            }
            else if (key == "Y-Shift Per Layer") {
                params.yShiftEnabled = valuePart.contains("True", Qt::CaseInsensitive);
            }
            else if (key == "Print Frequency") {
                params.printFrequency = valuePart.split(' ')[0].toDouble();
            }
            else if (key == "Calculated Print Speed (X-axis)") {
                params.printSpeed = valuePart.split(' ')[0].toDouble();
            }
            else if (key == "Droplet Spacing (X-axis resolution)") {
                params.dropletSpacingX = valuePart.split(' ')[0].toDouble();
            }
            else if (key == "Line Spacing (Y-axis resolution)") {
                params.lineSpacingY = valuePart.split(' ')[0].toDouble();
            }
            else if (key == "Nozzle Count") {
                params.nozzleCount = valuePart.split(' ')[0].toInt();
            }
            else if (key == "Part Position (Start X, Y)") {
                QStringList coords = valuePart.split(',');
                if (coords.size() == 2) {
                    params.startX = coords[0].remove("mm").toDouble();
                    params.startY = -coords[1].remove("mm").toDouble();
                }
            }
        }
    }
    file.close();

    // --- Log the parsed values for verification ---
    mPrinter->mjController->outputMessage("--- Parsed Print Parameters ---");
    mPrinter->mjController->outputMessage(QString("Source STL File: %1").arg(params.fileName));
    mPrinter->mjController->outputMessage(QString("Layer Height: %1 mm").arg(params.layerHeight));
    mPrinter->mjController->outputMessage(QString("Y-Shift Enabled: %1").arg(params.yShiftEnabled ? "True" : "False"));
    mPrinter->mjController->outputMessage(QString("Print Frequency: %1 Hz").arg(params.printFrequency));
    mPrinter->mjController->outputMessage(QString("Print Speed: %1 mm/s").arg(params.printSpeed));
    mPrinter->mjController->outputMessage(QString("Droplet Spacing (X): %1 mm").arg(params.dropletSpacingX));
    mPrinter->mjController->outputMessage(QString("Line Spacing (Y): %1 mm").arg(params.lineSpacingY));
    mPrinter->mjController->outputMessage(QString("Part Start X: %1 mm").arg(params.startX));
    mPrinter->mjController->outputMessage(QString("Part Start Y: %1 mm").arg(params.startY));
    mPrinter->mjController->outputMessage(QString("Nozzle Count: %1").arg(params.nozzleCount));
    mPrinter->mjController->outputMessage("-----------------------------");


    // Check if essential values were loaded.
    if (params.printFrequency == 0.0 || params.printSpeed == 0.0 || params.layerHeight == 0.0) {
        mPrinter->mjController->outputMessage("WARNING: Some critical print parameters might not have been parsed correctly.");
        return false;
    }

    return true;
}

// parses the layer_y_shifts.txt file for dithering
bool MJPrintheadWidget::parseLayerShifts(const QString& filePath, std::map<int, int>& shifts) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mPrinter->mjController->outputMessage(QString("ERROR: Could not open layer shifts file: %1").arg(filePath));
        return false;
    }

    QTextStream in(&file);
    if (!in.atEnd()) in.readLine(); // Skip header line "Layer,Y_Shift_Pixels"

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(',');
        if (parts.size() == 2) {
            bool ok1, ok2;
            int layer = parts[0].toInt(&ok1);
            int shift = parts[1].toInt(&ok2);
            if(ok1 && ok2) {
                shifts[layer] = shift;
            }
        }
    }
    file.close();
    mPrinter->mjController->outputMessage(QString("Successfully parsed layer shifts."));
    return true;
}

// main function for full prints of cut stl files
void MJPrintheadWidget::startFullPrintJob(const QString& jobFolderPath) {
    mPrinter->mjController->outputMessage(QString("--- Starting Full Print Job ---"));
    mPrinter->mjController->outputMessage(QString("Job Folder: %1").arg(jobFolderPath));

    // --- 1. Parse Parameter and Shift Files ---
    PrintParameters params;
    if (!parsePrintParameters(jobFolderPath + "\\print_parameters.txt", params)) {
        mPrinter->mjController->outputMessage(QString("FATAL: Failed to parse parameters. Aborting print."));
        return;
    }

    std::map<int, int> layerShifts;
    if (params.yShiftEnabled)
    {
        if (!parseLayerShifts(jobFolderPath + "\\layer_y_shifts.txt", layerShifts)) {
            mPrinter->mjController->outputMessage(QString("FATAL: Failed to parse layer shifts. Aborting print."));
            return;
        }
    }

    // --- 2. Get and Sort All Print Files ---
    QString dividedFolderPath = jobFolderPath + "\\divided";
    QDir dividedDir(dividedFolderPath);

    // Set filters to find only the bitmap files for the print layers.
    dividedDir.setNameFilters(QStringList() << "layer_*.bmp");
    dividedDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    // Set the sorting to QDir::Name, which sorts entries alphabetically. This is the key change.
    dividedDir.setSorting(QDir::Name);

    // Get the list of all matching filenames, now guaranteed to be in alphabetical order.
    QStringList fileList = dividedDir.entryList();

    if (fileList.isEmpty()) {
        mPrinter->mjController->outputMessage(QString("FATAL: No 'layer_*.bmp' files found in the divided folder. Aborting print."));
        return;
    }
    mPrinter->mjController->outputMessage(QString("Found %1 files to print in alphabetical order.").arg(fileList.count()));


    // --- 3. Pre-calculate Constant Values ---
    const int imageWidthPixels = static_cast<int>(ceil(100.0 / params.dropletSpacingX));

    // --- 4. Main Print Loop ---
    int lastLayerProcessed = -1; // Use -1 to indicate no layers have been processed yet.
    double layerBaseY = params.startY;

    // Iterate through the sorted list of files.
    for (const QString& fileName : fileList) {
        QString passFilePath = dividedDir.absoluteFilePath(fileName);

        // Use a regular expression to robustly parse layer and pass numbers from the filename.
        // This captures one or more digits (\d+) for the layer and pass numbers.
        QRegularExpression re("layer_(\\d+)_pass_(\\d+)\\.bmp");
        QRegularExpressionMatch match = re.match(fileName);

        if (!match.hasMatch()) {
            mPrinter->mjController->outputMessage(QString("WARNING: Skipping file with unexpected name format: %1").arg(fileName));
            continue; // Skip this file and move to the next one.
        }

        // Extract the layer and pass numbers from the matched parts of the filename.
        int currentLayer = match.captured(1).toInt();
        int currentPass = match.captured(2).toInt();

        // --- New Layer Detection ---
        // If the layer number from the current file is different from the last one we processed,
        // it means we are starting a new layer.
        if (currentLayer != lastLayerProcessed) {
            // If this isn't the very first layer, print the "Finished" message for the previous one.
            if (lastLayerProcessed != -1) {
                mPrinter->mjController->outputMessage(QString("--- Finished Layer %1 ---").arg(lastLayerProcessed));
            }

            mPrinter->mjController->outputMessage(QString("--- Starting Layer %1 ---").arg(currentLayer));

            // ===================================================================
            // === INSERT NEW LAYER CODE HERE ===
            // This is where you will add calls for:
            // 1. Lowering the Z-axis for the build plate.
            // 2. Depositing a new layer of powder.
            // 3. Rolling the powder to create a smooth surface.
            // ===================================================================
            mPrinter->mjController->outputMessage("Performing new layer operations (Z-move, powder, roll)...");
            GSleep(1000); // Placeholder delay for new layer operations

            // Calculate the base Y position for this new layer, accounting for any shifts.
            // This only needs to be calculated once per layer.
            if (params.yShiftEnabled) {
                int yPixelShift = layerShifts.count(currentLayer) ? layerShifts[currentLayer] : 0;
                double yShiftForLayer_mm = static_cast<double>(yPixelShift) * params.lineSpacingY;
                layerBaseY = params.startY - yShiftForLayer_mm;
            } else {
                layerBaseY = params.startY;
            }
            lastLayerProcessed = currentLayer;
        }

        // Calculate the specific Y location for the current pass within the layer.
        double yOffsetForPass_mm = static_cast<double>(currentPass - 1) * params.nozzleCount * params.lineSpacingY;
        double currentYLocation = layerBaseY + yOffsetForPass_mm;

        mPrinter->mjController->outputMessage(QString("Printing Layer %1, Pass %2 at Y=%3mm")
                                                  .arg(currentLayer).arg(currentPass).arg(currentYLocation));

        atLocation = false;
        // 06/30 TODO: setting startX to 0 because the offset is taken care of in the image
        // Call the print function with the full path to the current pass file. params.startX,
        printBMPatLocationEncoder(
            0.0,
            currentYLocation,
            params.printFrequency,
            params.printSpeed,
            imageWidthPixels,
            passFilePath
            );
        GSleep(100);
    }

    // After the loop finishes, print the "Finished" message for the very last layer.
    if (lastLayerProcessed != -1) {
        mPrinter->mjController->outputMessage(QString("--- Finished Layer %1 ---").arg(lastLayerProcessed));
    }

    mPrinter->mjController->outputMessage(QString("--- Print Job Complete ---"));
    moveNozzleOffPlate(); // Move nozzle to a safe position after the print
}


/* 6/9 thoughts and things to look at
!!!TODO: 6/9 work on implementing error catching including the printstartstop error catching so that all prints fit inside the build palte

Next steps:
- look at full error catching for prints
    - does the print fit inside the build box
    - is the safety factor too large (definitely currently)
- write some code so that after the full print finishes the head moves off the build plate to not drip
- look at how much the nozzle drips at different fill levels and write some code to verify interpolate good back pressure values
- implement simple x, y, z movement in the MJPrintheadWidget as well as simple back-pressure control so avoid moving between so many different screens
    - for X, Y, Z control: just forward and backwards motion as well as get current position and set speed
    - for back-pressure:
        - initially: toggle pressure up and down (for testing)
        - long term: set current liquid height and it will calculate the desired back pressure
- print some test images (added scientific logo)
- clean up the entirety of the code and remove any of the relics
- look at droplet velocity
- verify droplet volume
- write code to verify which nozzles are actually functioning
- 3d print holder for all components so none are lost or get dirty
*/





