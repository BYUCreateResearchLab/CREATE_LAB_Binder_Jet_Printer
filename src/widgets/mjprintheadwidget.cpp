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
#include <QFileDialog>
#include <QProcess>
#include <QFileInfo>
#include <QTextStream>
#include <map>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QProgressDialog>
#include <QApplication>


MJPrintheadWidget::MJPrintheadWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::MJPrintheadWidget),
    //  New addition for STL processing 06/24 !!! TODO
    m_pythonProcess(nullptr)
{
    ui->setupUi(this);

    setAccessibleName("Multi-Jet Printhead Widget");

    allow_widget_input_MJ(false);

    // Roller On Off Params
    m_isRollerOn = false;
    ui->rollerButton->setText("Roller Toggle \nOff");


    // --- 1. **Initialize and Connect Timers** ---
    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(100); // Set to 100ms interval
    connect(m_positionTimer, &QTimer::timeout, this, &MJPrintheadWidget::requestEncoderPosition);

    // --- 2. **Connect Printhead Control** ---
    connect(ui->connectButton, &QPushButton::clicked, this, &MJPrintheadWidget::connect_to_printhead);
    connect(ui->clearButton, &QPushButton::clicked, this, &MJPrintheadWidget::clear_response_text);
    connect(ui->inputLineEdit, &QLineEdit::returnPressed, this, &MJPrintheadWidget::command_entered);
    connect(ui->powerToggleButton, &QPushButton::clicked, this, &MJPrintheadWidget::powerTogglePressed);
    connect(ui->stopPrintingButton, &QPushButton::clicked, this, &MJPrintheadWidget::stopPrintingPressed);
    connect(ui->setFreqSpinBox, &QSpinBox::editingFinished, this, &MJPrintheadWidget::frequencyChanged);
    connect(ui->setVoltageSpinBox, &QSpinBox::editingFinished, this, &MJPrintheadWidget::voltageChanged);
    connect(ui->setStartSpinBox, &QSpinBox::editingFinished, this, &MJPrintheadWidget::absoluteStartChanged);
    connect(ui->imageFileLineEdit, &QLineEdit::returnPressed, this, &MJPrintheadWidget::file_name_entered);

    // --- 3. **Connect Status & Info Signals** ---
    connect(ui->getStatusButton, &QPushButton::clicked, this, [this]{mPrinter->mjController->report_status();});
    connect(ui->requestPHStatusPushButton, &QPushButton::clicked, this, [this]{mPrinter->mjController->request_status_of_all_heads();});
    connect(ui->getPositionButton, &QPushButton::clicked, this, &MJPrintheadWidget::getPositionPressed);
    connect(ui->getHeadTempButton, &QPushButton::clicked, this, &MJPrintheadWidget::getHeadTempsPressed);
    connect(mPrinter->mjController, &AsyncSerialDevice::response, this, &MJPrintheadWidget::write_to_response_window);

    // --- 4. **Connect Test & Utility** ---
    connect(ui->createBitmapButton, &QPushButton::clicked, this, &MJPrintheadWidget::createBitmapPressed);
    connect(ui->createTestBitmaps, &QPushButton::clicked, this, &MJPrintheadWidget::createTestBitmapsPressed);
    connect(ui->testPrintButton, &QPushButton::clicked, this, &MJPrintheadWidget::testPrintPressed);
    connect(ui->testJetButton, &QPushButton::clicked, this, &MJPrintheadWidget::testJetPressed);
    connect(ui->testNozzlesButton, &QPushButton::clicked, this, &MJPrintheadWidget::testNozzles);
    connect(ui->singleNozzlePushButton, &QPushButton::clicked, this, &MJPrintheadWidget::singleNozzlePressed);
    connect(ui->purgeNozzlesButton, &QPushButton::clicked, this, &MJPrintheadWidget::purgeNozzles);
    connect(ui->moveNozzle, &QPushButton::clicked, this, &MJPrintheadWidget::moveNozzleOffPlate);
    connect(ui->checkMaps, &QPushButton::clicked, this, &MJPrintheadWidget::checkMapsPressed);

    // --- 5. **Connect Encoder & Variable Print** ---
    connect(ui->zeroEncoder, &QPushButton::clicked, this, &MJPrintheadWidget::zeroEncoder);
    connect(ui->startStopDisplay, &QPushButton::clicked, this, &MJPrintheadWidget::onStartStopDisplayClicked);
    connect(ui->printVariableTestPrint, &QPushButton::clicked, this, &MJPrintheadWidget::variableTestPrintPressed);
    connect(ui->printVariableTestPrintEnc, &QPushButton::clicked, this, [this]() {
        encFlag = true;
        variableTestPrintPressed();
    });

    // --- 6. **Connect Motion Control** ---
    connect(ui->xRightButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::x_right_button_pressed_MJ);
    connect(ui->xLeftButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::x_left_button_pressed_MJ);
    connect(ui->yUpButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::y_up_button_pressed_MJ);
    connect(ui->yDownButtonMJ, &QAbstractButton::pressed, this, &MJPrintheadWidget::y_down_button_pressed_MJ);
    connect(ui->xRightButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);
    connect(ui->xLeftButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);
    connect(ui->yUpButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);
    connect(ui->yDownButtonMJ, &QAbstractButton::released, this, &MJPrintheadWidget::jog_released_MJ);
    connect(ui->xHomeMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_xHome_clicked_MJ);
    connect(ui->yHomeMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_yHome_clicked_MJ);
    connect(ui->zUpMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zUp_clicked_MJ);
    connect(ui->zDownMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zDown_clicked_MJ);
    connect(ui->zMaxMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zMax_clicked_MJ);
    connect(ui->zMinMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::on_zMin_clicked_MJ);
    connect(ui->zAbsoluteMoveButtonMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::move_z_to_absolute_position_MJ);
    connect(ui->getXAxisPositionMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::get_current_x_axis_position_MJ);
    connect(ui->getYAxisPositionMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::get_current_y_axis_position_MJ);
    connect(ui->getZAxisPositionMJ, &QAbstractButton::clicked, this, &MJPrintheadWidget::get_current_z_axis_position_MJ);

    // --- 7. **Connect Full Bed & Recoating** ---
    connect(ui->sliceStlButton, &QPushButton::clicked, this, &MJPrintheadWidget::sliceStlButton_clicked);
    connect(ui->startFullPrintButton, &QPushButton::clicked, this, &MJPrintheadWidget::on_startFullPrintButton_clicked);
    connect(ui->levelRecoatMJ, &QPushButton::clicked, this, &MJPrintheadWidget::levelRecoat_MJ);
    connect(ui->normalRecoatMJ, &QPushButton::clicked, this, &MJPrintheadWidget::normalRecoat_MJ);
    connect(ui->reRollLayerMJ, &QPushButton::clicked, this, &MJPrintheadWidget::reRollLayer);
    connect(ui->rollerButton, &QPushButton::clicked, this, &MJPrintheadWidget::onRollerButtonClicked);
}


MJPrintheadWidget::~MJPrintheadWidget()
{
    m_positionTimer->stop();
    delete ui;
}

// Enables or disables all motion and print-related widgets.
void MJPrintheadWidget::allow_widget_input(bool allowed)
{
    // --- 1. **Enable/Disable Jogging and Homing Buttons** ---
    ui->xRightButtonMJ->setEnabled(allowed);
    ui->xLeftButtonMJ->setEnabled(allowed);
    ui->yUpButtonMJ->setEnabled(allowed);
    ui->yDownButtonMJ->setEnabled(allowed);
    ui->xHomeMJ->setEnabled(allowed);
    ui->yHomeMJ->setEnabled(allowed);

    // --- 2. **Enable/Disable Z-Axis Controls** ---
    ui->zUpMJ->setEnabled(allowed);
    ui->zDownMJ->setEnabled(allowed);
    ui->zMaxMJ->setEnabled(allowed);
    ui->zMinMJ->setEnabled(allowed);
    ui->zAbsoluteMoveButtonMJ->setEnabled(allowed);

    // --- 3. **Enable/Disable Position Getters and Spin Boxes** ---
    ui->getXAxisPositionMJ->setEnabled(allowed);
    ui->getYAxisPositionMJ->setEnabled(allowed);
    ui->getZAxisPositionMJ->setEnabled(allowed);
    ui->xVelocityMJ->setEnabled(allowed);
    ui->yVelocityMJ->setEnabled(allowed);
    ui->zStepSizeMJ->setEnabled(allowed);
    ui->zAbsoluteMoveSpinBoxMJ->setEnabled(allowed);

    // --- 4. **Enable/Disable Test and Utility Buttons** ---
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
    ui->normalRecoatMJ->setEnabled(allowed);
    ui->levelRecoatMJ->setEnabled(allowed);
    ui->reRollLayerMJ->setEnabled(allowed);
    ui->rollerButton->setEnabled(allowed);
}

// Enables or disables widgets specific to the multi-jet printhead controller.
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

// Toggles the connection to the printhead controller board.
void MJPrintheadWidget::connect_to_printhead()
{
    if (mPrinter->mjController->is_connected())
    {
        allow_widget_input_MJ(true);
    }
    else
    {
        mPrinter->mjController->connect_board();
        allow_widget_input_MJ(true);
    }
}

void MJPrintheadWidget::clear_response_text()
{
    ui->responseTextEdit->clear();
}

// Sets the encoder's starting count to 1.
void MJPrintheadWidget::zeroEncoder()
{
    mPrinter->mjController->set_absolute_start(1);
}

// Requests the current encoder position from the controller.
void MJPrintheadWidget::requestEncoderPosition()
{
    if(mPrinter->mjController->is_connected()){
        mPrinter->mjController->report_current_position();
    }
}

// Starts or stops the timer that polls for encoder position.
void MJPrintheadWidget::onStartStopDisplayClicked()
{
    if (m_positionTimer->isActive())
    {
        // --- 1. **Stop the timer if it's active** ---
        m_positionTimer->stop();
        ui->startStopDisplay->setText("Start Encoder Display");
    }
    else
    {
        // --- 2. **Start the timer if the controller is connected** ---
        if (mPrinter->mjController->is_connected())
        {
            m_positionTimer->start();
            ui->startStopDisplay->setText("Stop Encoder Display");
        }
        else
        {
            mPrinter->mjController->outputMessage("Error: Controller is not connected.");
        }
    }
}

void MJPrintheadWidget::onRollerButtonClicked(){
    // Check the current state of the roller
    if (m_isRollerOn) {
        // If it's ON, turn it OFF.
        m_isRollerOn = false;

        QFont font = ui->rollerButton->font();
        font.setBold(false);
        ui->rollerButton->setFont(font);

        ui->rollerButton->setText("Roller Toggle \nOff"); // Set text for the *next* action

        std::stringstream s_roller;

        s_roller << CMD::disable_roller1();
        std::string returnStr = CMD::cmd_buf_to_dmc(s_roller);
        const char *cmds = returnStr.c_str();
        qDebug().noquote() << cmds;

        if (mPrinter->mcu->g) {
            GProgramDownload(mPrinter->mcu->g, cmds, "");
        }

        // --- 4. **Execute the compiled program** ---
        std::stringstream c_roller;
        c_roller << "GCmd," << "XQ" << "\n";
        c_roller << "GProgramComplete," << "\n";
        emit execute_command(c_roller);

        // --- Add your code here to actually stop the roller ---
        mPrinter->mjController->outputMessage("Roller has been turned OFF.");


    } else {
        // If it's OFF, turn it ON.
        m_isRollerOn = true;

        QFont font = ui->rollerButton->font();
        font.setBold(true);
        ui->rollerButton->setFont(font);
        ui->rollerButton->setText("Roller Toggle \nOn"); // Set text for the *next* action

        std::stringstream s_roller;

        s_roller << CMD::enable_roller1();
        std::string returnStr = CMD::cmd_buf_to_dmc(s_roller);
        const char *cmds = returnStr.c_str();
        qDebug().noquote() << cmds;

        if (mPrinter->mcu->g) {
            GProgramDownload(mPrinter->mcu->g, cmds, "");
        }

        // --- 4. **Execute the compiled program** ---
        std::stringstream c_roller;
        c_roller << "GCmd," << "XQ" << "\n";
        c_roller << "GProgramComplete," << "\n";
        emit execute_command(c_roller);

        // --- Add your code here to actually start the roller ---
        mPrinter->mjController->outputMessage("Roller has been turned ON.");
    }
}

// Sends the command from the input line edit.
void MJPrintheadWidget::command_entered()
{
    QString command = ui->inputLineEdit->text();
    send_command(command);
}

// Reads the filename from the image file line edit.
void MJPrintheadWidget::file_name_entered()
{
    QString filename = ui->imageFileLineEdit->text();
    read_in_file(filename); // loads in file for head 1
    read_in_file(filename, 2); // 11/24 loads in file for head 2
}


// Reads an image file and sends its data to the printhead controller.
void MJPrintheadWidget::read_in_file(const QString &fileNameOrPath, int headIdx)
{
    QString filePath = fileNameOrPath;
    QFileInfo fileInfo(filePath);

    // --- 1. **Prepend default directory if path is not absolute** ---
    if (!fileInfo.isAbsolute()) {
        const QString directory = "C:\\Users\\CB140LAB\\Desktop\\Noah\\";
        filePath = directory + fileNameOrPath;
    }

    QImage image(filePath);

    // --- 2. **Verify image loaded and send data** ---
    if (image.isNull())
    {
        mPrinter->mjController->emit response(QString("Failed to load image from " + filePath));
        return;
    }

    // --- 3. Calculate print head gap

    int whitespace = 0;

    if (headIdx == 2) {
        whitespace = 0;
    } else if (headIdx == 1) {
        whitespace = calculate_gap(fileNameOrPath);
        // check for failed calculation
        if (whitespace == -1) return;
    }

    mPrinter->mjController->send_image_data(headIdx, image, whitespace);
}

void MJPrintheadWidget::checkMapsPressed() {

    // CHANGE: Select a FILE, not a Directory.
    // The debug function needs to know exactly which layer/pass image you want to test.
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select Bitmap to Check"),
                                                    "C:\\Users\\CB140LAB\\Desktop\\Noah\\ComplexMultiNozzle\\Slicing",
                                                    tr("Images (*.bmp);;All Files (*)"));

    if (filePath.isEmpty()) return; // Handle Cancel

    QImage image(filePath);

    // --- 2. **Verify image loaded and send data** ---
    if (image.isNull())
    {
        mPrinter->mjController->emit response(QString("Failed to load image from " + filePath));
        return;
    }

    // --- 3. Calculate print head gap

    int whitespace1 = 0;

    int whitespace2 = calculate_gap(filePath);
    // check for failed calculation
    if (whitespace2 == -1) return;


    mPrinter->mjController->convert_image(1, image, whitespace1);
    mPrinter->mjController->convert_image(2, image, whitespace2);

}

// Toggles the power to the printhead.
void MJPrintheadWidget::powerTogglePressed()
{
    if (ui->powerToggleButton->isChecked())
        mPrinter->mjController->power_off();
    else
        mPrinter->mjController->power_on();
}

// Prints current encoder tick.
void MJPrintheadWidget::getPositionPressed()
{
    mPrinter->mjController->report_current_position();
}

void MJPrintheadWidget::getHeadTempsPressed()
{
    mPrinter->mjController->report_head_temps();
}

// Appends a command to the response window and sends it to the controller.
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
    mPrinter->mjController->set_head_voltage(Added_Scientific::Controller::HEAD2, volt); // 12/4 added second head. Is this right??
}

void MJPrintheadWidget::absoluteStartChanged()
{
    double steps = ui->setStartSpinBox->value();
    mPrinter->mjController->set_absolute_start(steps);
}

// Writes text from the controller to the appropriate response window.
void MJPrintheadWidget::write_to_response_window(const QString &text)
{
    const QString positionPrefix = "Encoder current count: ";

    // --- 1. **Check for and handle encoder position updates** ---
    if (text.startsWith(positionPrefix)){
        QString numberOnly = text.mid(positionPrefix.length());
        m_encoderHistory.append(numberOnly);

        // --- 2. **Maintain a history of the last 10 encoder readings** ---
        while(m_encoderHistory.size() > 10)
        {
            m_encoderHistory.removeFirst();
        }
        ui->responseTextEdit_2->setPlainText(m_encoderHistory.join(""));
    }
    // --- 3. **Display all other messages in the main response window** ---
    else{
        ui->responseTextEdit->appendPlainText(text);
    }
}

void MJPrintheadWidget::stopPrintingPressed()
{
    mPrinter->mjController->clear_nozzles();
}

// Moves the printhead to a safe location off the build plate.
void MJPrintheadWidget::moveNozzleOffPlate()
{
    // Getting the Current Y Position
    int currentYPos;
    GCmdI(mPrinter->mcu->g, "TPY", &currentYPos);
    double currentYPos_mm = currentYPos / (double)Y_CNTS_PER_MM;

    atLocation = false;
    moveToLocation(5.0, currentYPos_mm, QString("Moved off plate."));
    while (!atLocation) {
        QCoreApplication::processEvents();
    }
    GSleep(100);
}

// Executes a pre-defined test print routine.
void MJPrintheadWidget::testPrintPressed()
{
    // --- 1. **Set Printing Parameters** ---
    double printSpeed = 10.0;
    double truePrintStartX = 65.0;
    double printStartY = -65.0;
    double printFreq = 1000.0;
    double imageLength = 1000.0;
    double accelerationSpeed = 1000.0;

    // --- 2. **Calculate Distances & Coordinates** ---
    double safetyVal = 300.0; // Increased for testing visibility
    double backUpDistance = (pow(printSpeed, 2.0) / (2.0 * accelerationSpeed)) * safetyVal;
    double backUpDistanceEnc = backUpDistance * X_CNTS_PER_MM;
    double backedUpStartX = truePrintStartX - backUpDistance;
    double printDistance = (imageLength / printFreq) * printSpeed;
    double endTargetMM = backedUpStartX + backUpDistance + printDistance + backUpDistance;

    // --- 3. **Log Calculated Values** ---
    mPrinter->mjController->outputMessage(QString("--- Test Print Initiated ---"));
    mPrinter->mjController->outputMessage(QString("True Start (mm): %1").arg(truePrintStartX));
    mPrinter->mjController->outputMessage(QString("Backed-up Start (mm): %1").arg(backedUpStartX));
    mPrinter->mjController->outputMessage(QString("End Target (mm): %1").arg(endTargetMM));
    mPrinter->mjController->outputMessage(QString("Backup Distance (mm): %1").arg(backUpDistance));
    mPrinter->mjController->outputMessage(QString("Trigger Distance (counts): %1").arg(backUpDistanceEnc));
    mPrinter->mjController->outputMessage(QString(""));

    // --- 4. **Verify Start Alignment and Move to Acceleration Start** ---
    verifyPrintStartAlignment(truePrintStartX, printStartY);
    GSleep(2000); // Wait for verification print to finish.

    mPrinter->mjController->outputMessage(QString("Moving back to the acceleration start"));
    moveToLocation(backedUpStartX, printStartY, QString("Moved to Backed-Up Start"));

    while( !atLocation){
        QCoreApplication::processEvents();
    }
    atLocation = false;
    GSleep(100);

    // --- 5. **Prepare & Execute the Main Print Job** ---
    mPrinter->mjController->outputMessage(QString("Starting main print job..."));
    mPrinter->mjController->write_line("M 4"); // Set encoder print mode
    mPrinter->mjController->set_printing_frequency(printFreq);
    read_in_file("currentBitmap.bmp");
    //read_in_file("currentBitmap.bmp", 2);
    GSleep(100);
    mPrinter->mjController->set_absolute_start(backUpDistanceEnc);

    // --- 6. **Start Print and Wait for Completion** ---
    printComplete = false;
    printEnc(accelerationSpeed, printSpeed, endTargetMM, QString("Test Print Main Motion Complete"));

    while (!printComplete) {
        mPrinter->mjController->report_current_position();
        GSleep(100);
        QCoreApplication::processEvents();
    }
    printComplete = false;
    GSleep(80);

    mPrinter->mjController->outputMessage(QString("--- Test Print Finished ---"));
    moveNozzleOffPlate();
}

// Executes a test print of a logo using encoder-based triggering.
void MJPrintheadWidget::testJetPressed()
{
    // --- 1. **Set printing parameters** ---
    double xLocation = 50.0;
    double yLocation = 90.0;
    double frequency = 1500.0;
    double printSpeed = 10.0;
    int imageWidth = 1532;
    QString fileName = "mono_logo.bmp";

    // --- 2. **Call the encoder-based print function** ---
    printBMPatLocationEncoder(xLocation, yLocation, frequency, printSpeed, imageWidth, fileName);
}

void MJPrintheadWidget::createBitmapPressed()
{
    int numLines = ui->numLinesSpinBox->value();
    int width = ui->pixelWidthSpinBox->value();
    mPrinter->mjController->create_bitmap_lines(numLines, width);
}

// Configures the printhead to fire a single, specified nozzle for drop watching.
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

// Creates a set of bitmaps for testing various print parameters.
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

// Prints a series of bitmaps from a specified folder, extracting print parameters from each filename.
void MJPrintheadWidget::variableTestPrintPressed(){
    mPrinter->mjController->outputMessage(QString("--- Variable Test Print Started ---"));

    // --- 1. **Identify and list all bitmap files in the test folder** ---
    QString directoryPath = "C:\\Users\\CB140LAB\\Desktop\\Noah\\BitmapTestFolder";
    QDir dir(directoryPath);
    QStringList fileNames = dir.entryList(QDir::Files);
    mPrinter->mjController->outputMessage(QString("Found %1 files to print.").arg(fileNames.size()));

    if (!ui->powerToggleButton->isChecked()) {
        mPrinter->mjController->power_on();
    }

    // --- 2. **Iterate through each file and execute a print** ---
    for(const QString& fileName : fileNames){
        mPrinter->mjController->outputMessage(QString("\nProcessing file: %1").arg(fileName));

        // --- 3. **Load the image** ---
        QString fullPath = dir.filePath(fileName);
        QImage image(fullPath);
        if(image.isNull()){
            mPrinter->mjController->outputMessage(QString("Failed to load image: ") + fullPath );
            continue;
        }

        // --- 4. **Parse print parameters from the filename** ---
        QString frequencyStr = fileName.mid(fileName.length() - 8, 4);
        QString speedStr = fileName.mid(fileName.length() - 14, 5);
        QString rowStr = fileName.at(0);
        QString colStr = fileName.at(1);

        bool ok;
        int frequency = frequencyStr.toInt(&ok);
        if (!ok) { mPrinter->mjController->outputMessage("Failed to parse frequency."); continue; }
        double speed = speedStr.toDouble(&ok);
        if (!ok) { mPrinter->mjController->outputMessage("Failed to parse speed."); continue; }
        int row = rowStr.toInt(&ok);
        if (!ok) { mPrinter->mjController->outputMessage("Failed to parse row."); continue; }
        int col = colStr.toInt(&ok);
        if (!ok) { mPrinter->mjController->outputMessage("Failed to parse column."); continue; }

        // --- 5. **Set print location and parameters based on parsed values** ---
        double printStartX = 35.0 + (10.0 * col);
        double printStartY = 30.0 + (17.6 * row) * -1.0;
        QString fileNameWithFolder = "BitmapTestFolder\\" + fileName;

        // --- 6. **Execute the appropriate print function (encoder or immediate)** ---
        if(encFlag){
            printBMPatLocationEncoder(printStartX, printStartY, frequency, speed, image.width(), fileNameWithFolder);
        } else {
            printBMPatLocation(printStartX, printStartY, frequency, speed, image.width(), fileNameWithFolder);
        }
        GSleep(100);
    }
    encFlag = false;
    mPrinter->mjController->outputMessage(QString("--- Variable Test Print Finished ---"));
}

// Prints a bitmap at a specified location using immediate (non-encoder) triggering.
void MJPrintheadWidget::printBMPatLocation(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName){
    // --- 1. **Log Print Parameters** ---
    mPrinter->mjController->outputMessage(QString("--- Immediate Print Initiated ---"));
    mPrinter->mjController->outputMessage(QString("File: %1\n X: %2, Y: %3, Freq: %4, Speed: %5").arg(fileName).arg(xLocation).arg(yLocation).arg(frequency).arg(printSpeed));

    // --- 2. **Configure Printhead** ---
    mPrinter->mjController->write_line("M 3"); // Set immediate print mode
    mPrinter->mjController->set_printing_frequency(frequency);
    mPrinter->mjController->set_absolute_start(1);
    read_in_file(fileName);

    // --- 3. **Move to Start Location** ---
    moveToLocation(xLocation, yLocation, QString("Print BMP Start Location"));
    while (!atLocation) {
        QCoreApplication::processEvents();
    }
    atLocation = false;
    GSleep(80);

    // --- 4. **Execute Print Motion** ---
    double endTargetMM = xLocation + (imageWidth / frequency) * printSpeed;
    print(5000, printSpeed, endTargetMM, QString("Print BMP @ Location End"));
    while (!printComplete) {
        QCoreApplication::processEvents();
    }
    printComplete = false;
    GSleep(80);
    mPrinter->mjController->outputMessage(QString("--- Immediate Print Finished ---"));
}

// Prints a bitmap at a specified location using precise encoder-based triggering.
void MJPrintheadWidget::printBMPatLocationEncoder(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName){
    mPrinter->mjController->outputMessage(QString("--- Encoder-Based Print Initiated ---"));

    // --- 1. **Set Motion & Print Parameters** ---
    double accelerationSpeed = 3000.0;
    double safetyFactor = 2.0;
    //double encoderCountsPerMM = X_CNTS_PER_MM;

    // --- 2. Calculate Distances & Coordinates for Acceleration ---
    double backUpDistance = (pow(printSpeed, 2.0) / (2.0 * accelerationSpeed)) * safetyFactor;
    double printDistance = (static_cast<double>(imageWidth) / frequency) * printSpeed;

    double totalRunway = backUpDistance;

    double trueStartX = xLocation;
    double backedUpStartX = trueStartX - totalRunway;

    // Safety Clamp: Prevents negative X coordinates that stall the Galil
    if ((xLocation - totalRunway) < 0.0) {
        mPrinter->mjController->outputMessage(QString("WARNING: Start X too low. Shifting to 0.0mm"));
        xLocation = 0;
    }

    // endTargetMM must allow the trailing head (Head 1) to fully clear the image.
    double endTargetMM = backedUpStartX + totalRunway + printDistance + backUpDistance;

    // backUpDistanceEnc is the "Trigger Distance".
    // It is the number of encoder counts the carriage travels BEFORE jetting starts.
    // For Head 1 to hit 'trueStartX', the carriage travels 'totalRunway' mm.
    int backUpDistanceEnc = totalRunway * X_CNTS_PER_MM;

    // --- 3. **Log Calculated Values** ---
    mPrinter->mjController->outputMessage(QString("File: %1\n X: %2, Y: %3, Freq: %4, Speed: %5").arg(fileName).arg(xLocation).arg(yLocation).arg(frequency).arg(printSpeed));
    mPrinter->mjController->outputMessage(QString("Backed-up Start: %1 mm, End Target: %2 mm, Trigger: %3 counts").arg(backedUpStartX).arg(endTargetMM).arg(backUpDistanceEnc));

    // --- 4. **Configure Printhead and Load Data** ---
    mPrinter->mjController->write_line("M 4"); // Set encoder print mode
    mPrinter->mjController->set_printing_frequency(frequency);

    read_in_file(fileName); // HEAD 1
    if (!readyHeads()) return;
    read_in_file(fileName, 2); // HEAD 2
    if (!readyHeads()) return;
    GSleep(50);

    // --- 5. **Move to Backed-Up Starting Position** ---
    moveToLocation(backedUpStartX, yLocation, QString("Move to Encoder Start Complete"));
    while (!atLocation) {
        QCoreApplication::processEvents();
    }
    atLocation = false;
    GSleep(100);

    // --- 6. **Set Encoder Trigger and Execute Print** ---
    mPrinter->mjController->set_absolute_start(backUpDistanceEnc);
    printComplete = false;
    printEnc(accelerationSpeed, printSpeed, endTargetMM, QString("Encoder Print Motion Complete"));

    while (!printComplete) {
        QCoreApplication::processEvents();
    }
    printComplete = false;
    GSleep(100);

    mPrinter->mjController->outputMessage(QString("--- Encoder Print Finished ---"));
}

// Prints a small verification line at a precise location to check alignment.
void MJPrintheadWidget::verifyPrintStartAlignment(double xStart, double yStart){
    mPrinter->mjController->outputMessage(QString("Verifying print start alignment at X: %1, Y: %2").arg(xStart).arg(yStart));

    // --- 1. **Move to the exact start location** ---
    moveToLocation(xStart, yStart, QString("Test Print True Location"));
    while (!atLocation) {
        QCoreApplication::processEvents();
    }
    atLocation = false;
    GSleep(80);

    // --- 2. **Configure printhead for a single, short burst** ---
    mPrinter->mjController->write_line("M 4"); // Encoder mode
    mPrinter->mjController->set_printing_frequency(1000);
    mPrinter->mjController->set_absolute_start(1); // Trigger immediately
    read_in_file("LocationVerification.bmp");

    // --- 3. **Execute a very short print motion** ---
    printComplete = false;
    printEnc(1000, 10, xStart + 1.0, QString("Alignment Complete"));
    while (!printComplete) {
        QCoreApplication::processEvents();
    }
    printComplete = false;
    GSleep(80);
}

// Generates and executes commands to move the printhead to a specified X, Y location.
void MJPrintheadWidget::moveToLocation(double xLocation, double yLocation, QString endMessage){
    atLocation = false;
    mPrinter->mjController->outputMessage(QString("Moving to X: %1, Y: %2").arg(xLocation).arg(yLocation));

    std::stringstream s_cmdMove;

    // --- 1. **Build commands for Y-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::Y, 800);
    s_cmdMove << CMD::set_deceleration(Axis::Y, 800);
    s_cmdMove << CMD::set_speed(Axis::Y, 60);
    s_cmdMove << CMD::position_absolute(Axis::Y, yLocation);
    s_cmdMove << CMD::begin_motion(Axis::Y);
    s_cmdMove << CMD::after_motion(Axis::Y);

    // --- 2. **Build commands for X-axis movement** ---
    s_cmdMove << CMD::set_accleration(Axis::X, 600);
    s_cmdMove << CMD::set_deceleration(Axis::X, 600);
    s_cmdMove << CMD::set_speed(Axis::X, 60);
    s_cmdMove << CMD::position_absolute(Axis::X, xLocation);
    s_cmdMove << CMD::begin_motion(Axis::X);
    s_cmdMove << CMD::after_motion(Axis::X);

    // --- 3. **Add completion message and compile program** ---
    s_cmdMove << CMD::display_message("Arrived at Location");
    mPrinter->mjController->outputMessage(QString("End Message: %1").arg(endMessage));

    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmdMove);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g) {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // --- 4. **Execute the compiled program** ---
    std::stringstream c_cmdMove;
    c_cmdMove << "GCmd," << "XQ" << "\n";
    c_cmdMove << "GProgramComplete," << "\n";
    emit execute_command(c_cmdMove);
}

// Generates and executes commands for an immediate (non-encoder) print motion.
void MJPrintheadWidget::print(double acceleration, double speed, double endTargetMM, QString endMessage){
    printComplete = false;
    std::stringstream s_cmd;

    // --- 1. **Set motion parameters** ---
    s_cmd << CMD::set_accleration(Axis::X, acceleration);
    s_cmd << CMD::set_deceleration(Axis::X, acceleration);
    s_cmd << CMD::set_speed(Axis::X, speed);
    s_cmd << CMD::position_absolute(Axis::X, endTargetMM);

    // --- 2. **Set digital outputs to trigger immediate printing** ---
    s_cmd << CMD::start_MJ_print();
    s_cmd << CMD::start_MJ_dir();
    s_cmd << CMD::begin_motion(Axis::X);
    s_cmd << CMD::after_motion(Axis::X);

    // --- 3. **Clear digital outputs after motion** ---
    s_cmd << CMD::disable_MJ_dir();
    s_cmd << CMD::disable_MJ_start();

    // --- 4. **Add completion message and compile program** ---
    s_cmd << CMD::display_message("Print Complete");
    mPrinter->mjController->outputMessage(QString("End Message: %1").arg(endMessage));

    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g) {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // --- 5. **Execute the compiled program** ---
    std::stringstream c_cmd;
    c_cmd << "GCmd," << "XQ" << "\n";
    c_cmd << "GProgramComplete," << "\n";
    emit execute_command(c_cmd);
}

// Generates and executes commands for an encoder-based print motion.
void MJPrintheadWidget::printEnc(double acceleration, double speed, double endTargetMM, QString endMessage){
    printComplete = false;
    mPrinter->mjController->outputMessage(QString("Executing encoder print to %1mm at %2mm/s").arg(endTargetMM).arg(speed));
    std::stringstream s_cmd;

    // --- 1. **Set motion parameters for the print pass** ---
    s_cmd << CMD::set_accleration(Axis::X, acceleration);
    s_cmd << CMD::set_deceleration(Axis::X, acceleration);
    s_cmd << CMD::set_speed(Axis::X, speed);
    s_cmd << CMD::position_absolute(Axis::X, endTargetMM);
    s_cmd << CMD::begin_motion(Axis::X);
    s_cmd << CMD::after_motion(Axis::X);

    // --- 2. **Add completion message and compile program** ---
    s_cmd << CMD::display_message("Print Complete");
    mPrinter->mjController->outputMessage(QString("End Message: %1").arg(endMessage));

    std::string returnStr = CMD::cmd_buf_to_dmc(s_cmd);
    const char *cmds = returnStr.c_str();
    qDebug().noquote() << cmds;

    if (mPrinter->mcu->g) {
        GProgramDownload(mPrinter->mcu->g, cmds, "");
    }

    // --- 3. **Execute the compiled program** ---
    std::stringstream c_cmd;
    c_cmd << "GCmd," << "XQ" << "\n";
    c_cmd << "GProgramComplete," << "\n";
    emit execute_command(c_cmd);
}

// Executes an encoder-based purge sequence to clear nozzles.
void MJPrintheadWidget::purgeNozzles()
{
    // --- 1. **Define purge parameters** ---
    double truePurgeStartX = 10.0;
    double purgeY = 10.0;
    double purgeDistance = 5.0;
    double purgeSpeed = 10.0;
    double acceleration = 1000.0;
    double purgeFreq = 1000.0;

    mPrinter->mjController->outputMessage(QString("--- Encoder Purge Sequence Initiated ---"));

    if (!ui->powerToggleButton->isChecked()) {
        mPrinter->mjController->power_on();
        GSleep(100);
    }

    // --- 2. **Calculate distances (ignoring acceleration for this simple purge)** ---
    double backUpDistance = 1.0;
    double backedUpStartX = truePurgeStartX - backUpDistance;
    double endTargetMM = backedUpStartX + backUpDistance + purgeDistance + backUpDistance;

    // --- 3. **Configure the Printhead for Encoder-Based Printing** ---
    mPrinter->mjController->write_line("M 4");
    mPrinter->mjController->set_printing_frequency(purgeFreq);
    read_in_file("purge.bmp");

    // --- 4. **Move to the backed-up starting position** ---
    mPrinter->mjController->outputMessage(QString("Moving to purge start location..."));
    moveToLocation(backedUpStartX, purgeY, "Arrived at Backed-Up Purge Location");

    while (!atLocation) {
        QCoreApplication::processEvents();
    }
    atLocation = false;

    // --- 5. **Set the encoder trigger and execute the purge** ---
    mPrinter->mjController->set_absolute_start(1); // Trigger immediately after starting motion
    mPrinter->mjController->outputMessage(QString("Executing encoder-based purge..."));

    printComplete = false;
    printEnc(acceleration, purgeSpeed, endTargetMM, "Encoder Purge Motion Complete");

    while (!printComplete) {
        QCoreApplication::processEvents();
    }
    printComplete = false;
    GSleep(80);

    mPrinter->mjController->outputMessage(QString("--- Encoder Purge Sequence Finished ---"));
}

// Prints two test patterns to check nozzle health.
void MJPrintheadWidget::testNozzles()
{
    mPrinter->mjController->outputMessage(QString("--- Nozzle Test Initiated ---"));

    if (!ui->powerToggleButton->isChecked()) {
        mPrinter->mjController->power_on();
    }

    // --- 1. **Iterate and print two test files** ---
    for(int i = 0; i < 2; i++){
        printComplete = false;
        double printStartY = 200.0 + 20.0 * i;
        QString fileName = (i == 0) ? "nozzle_Test1.bmp" : "nozzle_Test2.bmp";

        // --- 2. **Call immediate print function for each pattern** ---
        printBMPatLocation(35.0, printStartY, 1000.0, 100.0, 1000, fileName);

        while (!printComplete) {
            QCoreApplication::processEvents();
        }
        printComplete = false;
        GSleep(80);
    }
    mPrinter->mjController->outputMessage(QString("--- Nozzle Test Finished ---"));
}

// Verifies if the print start and stop coordinates are within the build plate limits.
QString MJPrintheadWidget::verifyPrintStartStop(int xStart, int xStop){
    int xLowerLimit = 0;
    int xUpperLimit = 150;

    if(xStart < xLowerLimit) {
        return QString("ERROR: Print start is outside the build plate");
    } else if(xStop > xUpperLimit) {
        return QString("ERROR: Print stop is outside the build plate");
    } else {
        return QString("Print fits inside the build plate");
    }
}

// Launches a Python script to slice an STL file based on UI parameters.
void MJPrintheadWidget::sliceStlButton_clicked() {
    mPrinter->mjController->outputMessage(QString("Launching Python STL Slicer..."));

    // --- 1. Define executable and the Python GUI script path ---
    // IMPORTANT: Update this path to the final Python script you are using.
    QString pythonScriptPath = "C:\\Users\\CB140LAB\\Documents\\GitHub\\CREATE_LAB_Binder_Jet_Printer\\STL_Slicer_Dual.py";
    QString pythonExecutable = "C:\\Users\\CB140LAB\\AppData\\Local\\Microsoft\\WindowsApps\\python3.11.exe";

    // --- 2. Check if the process is already running ---
    if (m_pythonProcess && m_pythonProcess->state() == QProcess::Running) {
        mPrinter->mjController->outputMessage(QString("Slicer script is already running."));
        // Optionally, bring the existing window to the front if possible, or just return.
        return;
    }

    // --- 3. Setup and connect the QProcess for running the script ---
    m_pythonProcess = new QProcess(this);
    connect(m_pythonProcess, &QProcess::readyReadStandardOutput, this, &MJPrintheadWidget::readPythonOutput);
    connect(m_pythonProcess, &QProcess::readyReadStandardError, this, &MJPrintheadWidget::handlePythonError);
    connect(m_pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MJPrintheadWidget::onPythonScriptFinished);

    // --- 4. Launch the script ---
    // The Python script is a GUI app and no longer needs command-line arguments.
    QStringList args;
    args << pythonScriptPath;

    mPrinter->mjController->outputMessage(QString("Command: %1 %2").arg(pythonExecutable, args.join(" ")));
    m_pythonProcess->start(pythonExecutable, args);
}

void MJPrintheadWidget::readPythonOutput()
{
    QByteArray output = m_pythonProcess->readAllStandardOutput();
    qDebug() << "Python output:" << output;
}

void MJPrintheadWidget::handlePythonError()
{
    QByteArray errorOutput = m_pythonProcess->readAllStandardError();
    qWarning() << "Python error:" << errorOutput;
}

void MJPrintheadWidget::onPythonScriptFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Python script finished with exit code:" << exitCode;
    if (exitStatus == QProcess::CrashExit) {
        qWarning() << "The Python script crashed.";
    }
}


// --- Motion Control Functions ---

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

// --- Full Bed Printing Functions ---

// Prompts the user to select a print job folder and starts the print job.
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

// Parses the print_parameters.txt file to populate the print settings structure.
bool MJPrintheadWidget::parsePrintParameters(const QString& filePath, PrintParameters& params) {
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

            if (key == "Source STL File") params.fileName = valuePart;
            else if (key == "Layer Height (Z)") params.layerHeight = valuePart.split(' ')[0].toDouble();
            else if (key == "Y-Shift Per Layer") params.yShiftEnabled = valuePart.contains("True", Qt::CaseInsensitive);
            else if (key == "Print Frequency") params.printFrequency = valuePart.split(' ')[0].toDouble();
            else if (key == "Calculated Print Speed (X-axis)") params.printSpeed = valuePart.split(' ')[0].toDouble();
            else if (key == "Droplet Spacing (X-axis resolution)") params.dropletSpacingX = valuePart.split(' ')[0].toDouble();
            else if (key == "Line Spacing (Y-axis resolution)") params.lineSpacingY = valuePart.split(' ')[0].toDouble();
            else if (key == "Nozzle Count") params.nozzleCount = valuePart.split(' ')[0].toInt();
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

    // --- Log parsed values for verification ---
    mPrinter->mjController->outputMessage("--- Parsed Print Parameters ---");
    mPrinter->mjController->outputMessage(QString("Layer Height: %1 mm, Y-Shift: %2").arg(params.layerHeight).arg(params.yShiftEnabled ? "True" : "False"));
    mPrinter->mjController->outputMessage(QString("Freq: %1 Hz, Speed: %2 mm/s").arg(params.printFrequency).arg(params.printSpeed));
    mPrinter->mjController->outputMessage(QString("Start X: %1 mm, Start Y: %2 mm").arg(params.startX).arg(params.startY));
    mPrinter->mjController->outputMessage("-----------------------------");

    if (params.printFrequency == 0.0 || params.printSpeed == 0.0 || params.layerHeight == 0.0) {
        mPrinter->mjController->outputMessage("WARNING: Critical print parameters were not parsed correctly.");
        return false;
    }
    return true;
}

// Parses the parameter file to calculate the head gap in pixels (added 12/1)
int MJPrintheadWidget::calculate_gap(const QString &associatedBitmap) {
    // 1. Resolve path (Handle relative paths like "test.bmp")
    QString fullPath = associatedBitmap;
    QFileInfo fileInfo(fullPath);
    if (!fileInfo.isAbsolute()) {
        fullPath = "C:\\Users\\CB140LAB\\Desktop\\Noah\\" + associatedBitmap;
        fileInfo.setFile(fullPath);
    }

    // 2. Find Parameter File
    QDir dir = fileInfo.absoluteDir();
    QString paramPath = dir.filePath("print_parameters.txt");

    // Look in current folder, then parent folder
    if (!QFile::exists(paramPath)) {
        if (dir.cdUp()) {
            QString parentPath = dir.filePath("print_parameters.txt");
            if (QFile::exists(parentPath)) paramPath = parentPath;
        }
    }

    // 3. Parse and Error Check
    PrintParameters params;
    if (!QFile::exists(paramPath) || !parsePrintParameters(paramPath, params))
    {
        // ERROR: No defaults allowed. Stop everything.
        mPrinter->mjController->outputMessage(QString("ERROR: Could not find 'print_parameters.txt' for file: %1").arg(associatedBitmap));
        return -1; // Return error code
    }

    // 4. Calculate
    if (params.dropletSpacingX <= 0) return -1; // Avoid divide by zero

    // Use the constant from printer.h
    return static_cast<int>(HEAD_GAP_MM / params.dropletSpacingX);
}

// Parses the layer_y_shifts.txt file to load dithering values for each layer.
bool MJPrintheadWidget::parseLayerShifts(const QString& filePath, std::map<int, int>& shifts) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mPrinter->mjController->outputMessage(QString("ERROR: Could not open layer shifts file: %1").arg(filePath));
        return false;
    }

    QTextStream in(&file);
    if (!in.atEnd()) in.readLine(); // Skip header line

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

// Main function for executing a multi-layer print from a sliced STL job folder.
void MJPrintheadWidget::startFullPrintJob(const QString& jobFolderPath) {
    mPrinter->mjController->outputMessage(QString("--- Starting Full Print Job from folder: %1 ---").arg(jobFolderPath));
    m_printJobCancelled = false; // Reset cancellation flag

    // --- 1. **Parse Parameter and Shift Files** ---
    PrintParameters params;
    if (!parsePrintParameters(jobFolderPath + "\\print_parameters.txt", params)) {
        mPrinter->mjController->outputMessage("FATAL: Failed to parse parameters. Aborting print.");
        return;
    }
    std::map<int, int> layerShifts;
    if (params.yShiftEnabled && !parseLayerShifts(jobFolderPath + "\\layer_y_shifts.txt", layerShifts)) {
        mPrinter->mjController->outputMessage("FATAL: Failed to parse layer shifts. Aborting print.");
        return;
    }

    // --- 2. **Get and Alphabetically Sort All Print Files** ---
    QDir dividedDir(jobFolderPath + "\\divided");
    dividedDir.setNameFilters(QStringList() << "layer_*.bmp");
    dividedDir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    dividedDir.setSorting(QDir::Name); // Sort alphabetically to ensure correct order
    QStringList fileList = dividedDir.entryList();

    if (fileList.isEmpty()) {
        mPrinter->mjController->outputMessage("FATAL: No 'layer_*.bmp' files found. Aborting print.");
        return;
    }
    mPrinter->mjController->outputMessage(QString("Found %1 files to print.").arg(fileList.count()));

    // Calculate the total number of layers ---
    int totalLayers = 0;
    if (!fileList.isEmpty()) {
        QRegularExpression re("layer_(\\d+)_pass_(\\d+)\\.bmp");
        QRegularExpressionMatch match = re.match(fileList.last());
        if (match.hasMatch()) {
            totalLayers = match.captured(1).toInt();
        }
    }

    // --- 3. **Setup and Show the Progress Dialog** ---
    m_printStatusDialog = new QProgressDialog("Starting print job...", "Cancel", 0, totalLayers, this); // Use totalLayers
    m_printStatusDialog->setWindowTitle("Print Job Status");
    m_printStatusDialog->setWindowModality(Qt::WindowModal);
    m_printStatusDialog->setMinimumDuration(0);
    connect(m_printStatusDialog, &QProgressDialog::canceled, this, &MJPrintheadWidget::cancelPrintJob);
    m_printStatusDialog->show();
    QApplication::processEvents();


    // --- 4. **Main Print Loop** ---
    int lastLayerProcessed = -1;
    double layerBaseY = params.startY;

    for (const QString& fileName : fileList) {
        if (m_printJobCancelled) break; // Check for cancellation at the start of each pass

        // --- 4a. Update Progress and Parse Filename ---
        QApplication::processEvents();
        QRegularExpression re("layer_(\\d+)_pass_(\\d+)\\.bmp");
        QRegularExpressionMatch match = re.match(fileName);
        if (!match.hasMatch()) {
            mPrinter->mjController->outputMessage(QString("WARNING: Skipping file with unexpected name: %1").arg(fileName));
            continue;
        }
        int currentLayer = match.captured(1).toInt();
        int currentPass = match.captured(2).toInt();
        m_printStatusDialog->setLabelText(QString("Layer %1 / %2\nPrinting Pass: %3")
                                              .arg(currentLayer)
                                              .arg(totalLayers)
                                              .arg(currentPass));        QApplication::processEvents();

        // --- 4b. New Layer Detection and Operations ---
        if (currentLayer != lastLayerProcessed) {
            m_printStatusDialog->setValue(currentLayer);

            if (lastLayerProcessed != -1) {
                mPrinter->mjController->outputMessage(QString("--- Finished Layer %1 ---").arg(lastLayerProcessed));
            }
            mPrinter->mjController->outputMessage(QString("--- Starting Layer %1 ---").arg(currentLayer));

            // Perform recoat for all layers after the first one
            if (currentLayer > 1) {
                // 1. Move nozzle to park position FIRST.
                mPrinter->mjController->outputMessage("Moving nozzle to park position for recoat.");

                moveNozzleOffPlate();

                // 2. Now that the head is parked, perform the recoat operation.
                mPrinter->mjController->outputMessage("Performing recoat operation...");
                recoatComplete = false;
                performRecoat(&params, true);
                while (!recoatComplete) {
                    QCoreApplication::processEvents();
                }
            } else {
                recoatComplete = true; // Skip recoat for the very first layer
            }

            // Calculate base Y position for the new layer, including any dithering shift
            if (params.yShiftEnabled) {
                int yPixelShift = layerShifts.count(currentLayer) ? layerShifts[currentLayer] : 0;
                double yShiftForLayer_mm = static_cast<double>(yPixelShift) * params.lineSpacingY;
                layerBaseY = params.startY - yShiftForLayer_mm;
            } else {
                layerBaseY = params.startY;
            }
            lastLayerProcessed = currentLayer;
        }

        // --- 4c. Calculate Y Location for Current Pass and Print ---
        double yOffsetForPass_mm = static_cast<double>(currentPass - 1) * params.nozzleCount * params.lineSpacingY;
        double currentYLocation = layerBaseY - yOffsetForPass_mm;

        mPrinter->mjController->outputMessage(QString("Printing Pass %1 at Y=%2mm").arg(currentPass).arg(currentYLocation));

        const int imageWidthPixels = static_cast<int>(ceil(100.0 / params.dropletSpacingX));
        QString passFilePath = dividedDir.absoluteFilePath(fileName);

        printBMPatLocationEncoder(
            params.startX,// check this
            (currentYLocation - Y_HEAD_OFFSET),
            params.printFrequency,
            params.printSpeed,
            imageWidthPixels,
            passFilePath
            );
        GSleep(100);
    }

    // --- 5. **Post-Print Cleanup** ---
    if (m_printStatusDialog) {
        m_printStatusDialog->close();
        delete m_printStatusDialog;
        m_printStatusDialog = nullptr;
    }

    if (m_printJobCancelled) {
        mPrinter->mjController->outputMessage(QString("--- PRINT JOB CANCELLED BY USER ---"));
    } else {
        if (lastLayerProcessed != -1) {
            mPrinter->mjController->outputMessage(QString("--- Finished Layer %1 ---").arg(lastLayerProcessed));
        }
        mPrinter->mjController->outputMessage(QString("--- Print Job Complete ---"));
    }
    moveNozzleOffPlate();
}

// Does a Level Recoat (this is used to create a smooth top layer without moving the Z)
void MJPrintheadWidget::levelRecoat_MJ()
{
    std::stringstream s;
    RecoatSettings levelRecoat{};
    levelRecoat.isLevelRecoat = true;
    levelRecoat.rollerTraverseSpeed_mm_s = ui->rollerTraverseSpeedSpinBoxMJ->value();
    levelRecoat.recoatSpeed_mm_s = ui->recoatSpeedSpinBoxMJ->value();
    levelRecoat.ultrasonicIntensityLevel = ui->ultrasonicIntensityComboBoxMJ->currentIndex();
    levelRecoat.ultrasonicMode = ui->ultrasonicModeComboBoxMJ->currentIndex();
    levelRecoat.layerHeight_microns = ui->layerHeightSpinBoxMJ->value();
    levelRecoat.waitAfterHopperOn_millisecs = ui->hopperDwellTimeMsSpinBox->value();

    s << CMD::display_message("starting level recoat...");
    for(int i{0}; i < ui->recoatCyclesSpinBoxMJ->value(); ++i)
    {
        s << CMD::display_message("spreading layer " + std::to_string(i+1) + "...");
        s << CMD::spread_layer(levelRecoat);
    }
    s << CMD::display_message("powder spreading complete");
    s << CMD::move_xy_axes_to_default_position();

    emit execute_command(s);
    emit generate_printing_message_box("Level recoat is in progress.");
}

// Does a Normal Recoat (the Z is lowered one layer height and powder rolled)
void MJPrintheadWidget::normalRecoat_MJ()
{
    std::stringstream s;
    RecoatSettings layerRecoatSettings {};
    layerRecoatSettings.isLevelRecoat = false;
    layerRecoatSettings.rollerTraverseSpeed_mm_s = ui->rollerTraverseSpeedSpinBoxMJ->value();
    layerRecoatSettings.recoatSpeed_mm_s = ui->recoatSpeedSpinBoxMJ->value();
    layerRecoatSettings.ultrasonicIntensityLevel = ui->ultrasonicIntensityComboBoxMJ->currentIndex();
    layerRecoatSettings.ultrasonicMode = ui->ultrasonicModeComboBoxMJ->currentIndex();
    layerRecoatSettings.layerHeight_microns = ui->layerHeightSpinBoxMJ->value();
    layerRecoatSettings.waitAfterHopperOn_millisecs = ui->hopperDwellTimeMsSpinBox->value();

    s << CMD::display_message("starting normal recoat...");
    for(int i{0}; i < ui->recoatCyclesSpinBoxMJ->value(); ++i)
    {
        s << CMD::display_message("spreading layer " + std::to_string(i+1) + "...");
        s << CMD::spread_layer(layerRecoatSettings);
    }
    s << CMD::display_message("powder spreading complete");
    s << CMD::move_xy_axes_to_default_position();

    emit execute_command(s);
    emit generate_printing_message_box("Normal recoat is in progress.");
}

// Executes a single recoat cycle, using either UI settings or parsed print parameters.
void MJPrintheadWidget::performRecoat(const PrintParameters* params, bool usePrintParameters)
{
    std::stringstream s;
    RecoatSettings recoatSettings{};

    // --- 1. **Get settings from UI or parsed parameters** ---
    if (usePrintParameters && params) {
        recoatSettings.layerHeight_microns = static_cast<int>(params->layerHeight * 1000);
    } else {
        recoatSettings.layerHeight_microns = ui->layerHeightSpinBoxMJ->value();
    }
    recoatSettings.isLevelRecoat = false; // Normal recoat for full prints
    recoatSettings.rollerTraverseSpeed_mm_s = ui->rollerTraverseSpeedSpinBoxMJ->value();
    recoatSettings.recoatSpeed_mm_s = ui->recoatSpeedSpinBoxMJ->value();
    recoatSettings.ultrasonicIntensityLevel = ui->ultrasonicIntensityComboBoxMJ->currentIndex();
    recoatSettings.ultrasonicMode = ui->ultrasonicModeComboBoxMJ->currentIndex();
    recoatSettings.waitAfterHopperOn_millisecs = ui->hopperDwellTimeMsSpinBox->value();

    // --- 2. **Build and execute the recoat command** ---
    s << CMD::display_message("Recoating for new layer...");
    s << CMD::spread_layer(recoatSettings);
    s << CMD::display_message("Recoat Complete");

    emit execute_command(s);
}

// Executes a single recoat cycle, using either UI settings or parsed print parameters.
void MJPrintheadWidget::reRollLayer()
{
    std::stringstream s;
    Axis y {Axis::Y};
    double zAxisOffsetUnderRoller {0.5};

    // move z-axis down when going back to get more powder
    s << CMD::set_accleration(Axis::Z, 10)
      << CMD::set_deceleration(Axis::Z, 10)
      << CMD::set_speed(Axis::Z, 2)
      << CMD::position_relative(Axis::Z, -zAxisOffsetUnderRoller)
      << CMD::begin_motion(Axis::Z)
      << CMD::motion_complete(Axis::Z);

    // jog y-axis to back
    s << CMD::set_accleration(y, 400)
      << CMD::set_deceleration(y, 400)
      << CMD::set_jog(y, -50)
      << CMD::begin_motion(y)
      << CMD::motion_complete(y);

    s << CMD::position_relative(Axis::Z, zAxisOffsetUnderRoller);

    // set hopper settings

    // move z-axis
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);

    // move y-axis forward
    s << CMD::set_deceleration(y, 1000);
    s << CMD::set_accleration(y, 1000);
    s << CMD::set_speed(y, 150);
    s << CMD::position_relative(y, 80);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    // enable rollers
    s << CMD::enable_roller1();
    s << CMD::enable_roller2();

    // move y-axis forward under roller
    s << CMD::set_speed(y, 6);
    s << CMD::position_relative(y, 175);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    // turn off rollers
    s << CMD::disable_roller1();
    s << CMD::disable_roller2();

    s << CMD::set_jog(y, 50);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    emit execute_command(s);
    emit generate_printing_message_box("Surface Re-Roll is in progress.");
}


// Sets the flag to cancel an ongoing multi-layer print job.
void MJPrintheadWidget::cancelPrintJob()
{
    mPrinter->mjController->outputMessage("--- CANCELLATION REQUESTED ---");
    m_printJobCancelled = true;
    if (m_printStatusDialog) {
        m_printStatusDialog->setLabelText("Cancelling print job, please wait...");
        m_printStatusDialog->setCancelButton(nullptr); // Disable button after click
    }
}


#include <QEventLoop>
#include <QTimer>

bool MJPrintheadWidget::readyHeads()
{
    // Define the good statuses (only 10 likely)
    const QString STATUS_READY = "10";

    // Wait for status message
    for (int attempt = 0; attempt < 2; attempt++) {

        QEventLoop loop;
        QString lastStatus = "";
        bool statusReceived = false;

        // Listen for status response
        QMetaObject::Connection conn = connect(mPrinter->mjController, &AsyncSerialDevice::response,
                                               [&](const QString &response) {
                                                   // // Filter response
                                                   if (response.contains(STATUS_READY) || response.contains("-") || response.toInt() != 0) {
                                                       lastStatus = response.trimmed(); // Remove whitespace
                                                       statusReceived = true;
                                                       loop.quit();
                                                   }
                                               });

        // Request Status
        mPrinter->mjController->request_status_of_all_heads();

        // Wait (Timeout 2 seconds)
        QTimer::singleShot(2000, &loop, &QEventLoop::quit);
        loop.exec();
        disconnect(conn);

        // Evaluate stauts
        if (!statusReceived) {
            mPrinter->mjController->outputMessage("Warning: Timeout waiting for status.");
            continue; // Retry loop
        }

        if (lastStatus.contains(STATUS_READY)) {
            return true; // Success
        }
        else {
            // If not active
            mPrinter->mjController->outputMessage(QString("Error: Invalid Status '%1'. Initiating Auto-Power Cycle...").arg(lastStatus));

            // AUTO RECOVERY SEQUENCE
            mPrinter->mjController->power_off();
            mPrinter->mjController->outputMessage("Heads off...");

            // Wait for drain
            QEventLoop waitLoop;
            QTimer::singleShot(2000, &waitLoop, &QEventLoop::quit);
            waitLoop.exec();

            mPrinter->mjController->power_on();
            mPrinter->mjController->outputMessage("Heads on...");

            // Wait for boot
            QTimer::singleShot(2000, &waitLoop, &QEventLoop::quit);
            waitLoop.exec();

            // RE-APPLY SETTINGS
            int freq = ui->setFreqSpinBox->value();
            double volt = ui->setVoltageSpinBox->value(); // Take voltage from UI
            mPrinter->mjController->set_printing_frequency(freq);
            mPrinter->mjController->set_head_voltage(Added_Scientific::Controller::HEAD1, volt);
            mPrinter->mjController->set_head_voltage(Added_Scientific::Controller::HEAD2, volt);

            mPrinter->mjController->outputMessage("Recovery Complete. Retrying status check...");
        }
    }

    mPrinter->mjController->outputMessage("CRITICAL: Unable to recover head status 10.");
    return false;
}
/* 6/9 thoughts and things to look at
!!!TODO: 6/9 work on implementing error catching including the printstartstop error catching so that all prints fit inside the build palte

Next steps:
- DONE!!!  look at full error catching for prints
    - DONE!!! does the print fit inside the build box
    - DONE!!! is the safety factor too large (definitely currently)
- DONE!!! write some code so that after the full print finishes the head moves off the build plate to not drip
- look at how much the nozzle drips at different fill levels and write some code to verify interpolate good back pressure values
- implement simple x, y, z movement in the MJPrintheadWidget as well as simple back-pressure control so avoid moving between so many different screens
    - DONE!!! for X, Y, Z control: just forward and backwards motion as well as get current position and set speed
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
