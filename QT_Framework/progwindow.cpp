
#include "progwindow.h"
#include "ui_progwindow.h"
//#include "JetServer.h"
#include <math.h>
#include <iostream>


using namespace std;

int jetter_setup();

progWindow::progWindow(QWidget *parent) :
    QWidget(parent),

  ui(new Ui::progWindow) {
        ui->setupUi(this);

        // Internal Table Data Storage Setup
        table.addRows(1); // add 1 row (set) to start program

        // UI Table Setup
        ui->tableWidget->clear(); // clear the table
        updateTable(true, true);

        // Misc. Setup
        ui->numSets->setValue(1);
        ui->startX->setValue(table.startX);
        ui->startY->setValue(table.startY);
        ui->setSpacing->setValue(table.setSpacing);

        //SVG Viewer setup
        ui->SVGViewer->setup();
        updatePreviewWindow();
}

progWindow::~progWindow()
{
    delete ui;
}


void progWindow::log(QString message, enum logType messageType = logType::Standard)
{
    // If current message type is an active log type
    if(std::find(activeLogTypes.begin(), activeLogTypes.end(), messageType) != activeLogTypes.end()) {
        ui->consoleOutput->insertPlainText(message + "\n");
    }
}


void progWindow::updatePreviewWindow()
{
    std::vector<QLineF> lines = table.qLines(); // vector of lines to add to window
    ui->SVGViewer->scene()->clear(); // clear the window
    for(size_t i=0; i < lines.size(); i++){ // for each line
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
}


void progWindow::CheckCell(int row, int column)
{
    QString cellText = ui->tableWidget->item(row, column)->text();

    if(cellText == table.data[row][column].toQString()){
        //Log("Value to check is already stored in the table", logType::Debug);
        return; // break if the UI table value is already stored in the internal table
    }

    log(QString::fromStdString("Checking row " + std::to_string(row) +
                               " and column " + std::to_string(column)), logType::Debug);
    log("value checked was \"" + cellText + "\"", logType::Debug);

    errorType error_state = table.data[row][column].updateData(cellText); // Update cell value and get back error code
    switch(error_state){
    case errorNone: // no error
        log("The conversion to int was successful and read as " +
            QString::number(table.data[row][column].value), logType::Debug);
        break;
    case errortooSmall: // number was too small and set to min
        log("The number entered is too small\nThe min value is " +
            QString::number(table.data[row][column].min), logType::Error);
        break;
    case errortooLarge: // number was too large and set to max
        log("The number entered is too large\nThe max value is " +
            QString::number(table.data[row][column].max), logType::Error);
        break;
    case errorCannotConvert: // Could not convert QString to valid type
        log("There was an error with the data. The value was cleared", logType::Error);
        break;
    default:
        break;
    }

    // Update cell text for user interface
    ui->tableWidget->item(row, column)->setText(table.data[row][column].toQString());
}


void progWindow::updateCell(int row, int column)
{
    // add new widget item to empty cells and change text for existing cells
    QTableWidgetItem* item = ui->tableWidget->item(row,column);
    if(!item || item->text().isEmpty()){
        ui->tableWidget->setItem(row,column, new QTableWidgetItem(table.data[row][column].toQString()));
    }else{
        item->setText(table.data[row][column].toQString());
    }
}

void progWindow::updateTable(bool updateVerticalHeaders = false, bool updateHorizontalHeaders = false)
{
    // Set Table Size
    ui->tableWidget->setRowCount(table.data.size());
    ui->tableWidget->setColumnCount(table.data[0].size);

    // Update Vertical Headers if updateVerticalHeaders is set to true
    if(updateVerticalHeaders){
        QStringList verticalHeaders = QStringList(); // Create new string list for setting vertical headers
        for(int i = 0; i < ui->tableWidget->rowCount(); i++){ // For each row
            verticalHeaders.append(QString::fromStdString("Set " + std::to_string(i + 1))); // Add string to list
        }
        ui->tableWidget->setVerticalHeaderLabels(verticalHeaders);
    }

    // Update Horizontal Headers if updateHorizontalHeaders is set to true
    if(updateHorizontalHeaders){
        QStringList horizontalHeaders = QStringList(); // Create new string list for setting vertical headers
        for(int i = 0; i < ui->tableWidget->columnCount(); i++){ // For each row
            horizontalHeaders.append(QString::fromStdString(table.data[0][i].typeName)); // Add string to list
        }
        ui->tableWidget->setHorizontalHeaderLabels(horizontalHeaders);
        ui->tableWidget->horizontalHeader()->setVisible(true);
    }

    // Replace cell text for each cell in UI Table
    for(int r = 0; r < ui->tableWidget->rowCount(); r++){
        for(int c = 0; c < ui->tableWidget->columnCount(); c++){
            updateCell(r,c);
        }
    }
}


/**************************************************************************
 *                                SLOTS                                   *
 **************************************************************************/


void progWindow::on_back2Home_clicked()
{
    this->close();
    emit firstWindow();
}


void progWindow::on_numSets_valueChanged(int rowCount)
{
    int prevRowCount = table.data.size(); // get previous row count
    // set new row count
    if(rowCount > prevRowCount){ // If adding rows
        table.addRows(rowCount - prevRowCount);
        updateTable(true,false);
        updatePreviewWindow();
    }
    if(rowCount < prevRowCount){ // If removing rows
        table.removeRows(prevRowCount - rowCount);
        updateTable(true,false);
        updatePreviewWindow();
    }
}


void progWindow::on_tableWidget_cellChanged(int row, int column)
{
    CheckCell(row, column); // Check the cell that was changed
    updatePreviewWindow();
    ui->consoleOutput->ensureCursorVisible(); // Scroll to new content on console
}


void progWindow::on_startX_valueChanged(double arg1)
{
    table.startX = arg1;
    updatePreviewWindow();
}


void progWindow::on_startY_valueChanged(double arg1)
{
    table.startY = arg1;
    updatePreviewWindow();
}


void progWindow::on_setSpacing_valueChanged(double arg1)
{
    table.setSpacing = arg1;
    updatePreviewWindow();
}


void progWindow::on_printPercentSlider_sliderMoved(int position)
{
    double percent = (double)position / (double)ui->printPercentSlider->maximum();


    std::vector<QLineF> lines = table.qLines(); // vector of lines to add to window
    ui->SVGViewer->scene()->clear(); // clear the window

    int numLinestoShow = lines.size() * percent;

    for(int i=0; i < numLinestoShow; i++){ // for each line
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
}


void progWindow::on_clearConsole_clicked()
{
    ui->consoleOutput->clear();
}


void progWindow::on_startPrint_clicked()
{
    //GO TO XSTART, YSTART, ZMAX
    if(g ==0) {
        progWindow::connectToController();
    }

    spread_x_layers(ui->layersToSpread->value());

    //Print Sets
    for(int i = 0; i < int(table.numRows()); ++i) {
        printLineSet(i);
    }

    /*
    //GO TO HOME AFTER PRINT
    string xHomeString = "PAX=" + to_string(75000 - ((50-table.startX)*1000));
    e(GCmd(g, xHomeString.c_str()));
    string yHomeString = "PAY=" + to_string(table.startY*800);
    e(GCmd(g, yHomeString.c_str()));
    e(GCmd(g, "BGX"));
    e(GCmd(g, "BGY"));
    e(GMotionComplete(g, "X")); // Wait until limit is reached
    e(GMotionComplete(g, "Y"));
    */
}

void progWindow::printLineSet(int setNum) {
    //Find starting position for line set
    float x_start = table.startX;
    for(int i = 0; i < setNum; ++i) {
        x_start = x_start + table.data[i].lineLength.value + table.setSpacing;
    }
    float y_start = table.startY;

    e(GCmd(g, "SPY=25000")); // 31.25 mm/s (Put this here for now, but we will want a setting for y step speed during line printing)

    //calculate line specifics - TODO: CURRENTLY OVERCONSTRAINED

    //GO TO BEGINNING POINT
    if(g) {
        //e(GCmd(g, "DPX=75000"));
        //e(GCmd(g, "DPY=0"));
        string xHomeString = "PAX=" + to_string(75000 - ((50-x_start)*1000));
        e(GCmd(g, xHomeString.c_str()));
        string yHomeString = "PAY=" + to_string(y_start*800);
        e(GCmd(g, yHomeString.c_str()));
        e(GCmd(g, "BGX"));
        e(GCmd(g, "BGY"));
        e(GMotionComplete(g, "X")); // Wait until limit is reached
        e(GMotionComplete(g, "Y"));
    }

    //START LINE PRINTING HERE!
    if(g) {
        float curY = y_start;
        float curX = x_start;
        for(int i = 0; i < table.data[setNum].numLines.value; ++i)
        {

            //STEP DROPPING VERSION
            /*
            while(curX < (x_start + (table.data[setNum].lineLength.value)))
            {
                string xString = "PAX=" + to_string(75000 - ((50-curX)*1000));
                e(GCmd(g, xString.c_str()));
                e(GCmd(g, "BGX"));
                e(GMotionComplete(g, "X"));

                //Jet command here

                //Update X Value
                curX = curX + (floor(table.data[setNum].dropletSpacing.value/5)); //TODO : Fix here to move smaller increments than 1 mm
            }
            */

            //CONTINUOUS MOTION VERSION
            //Change velocity depending on the table inputs
            //enable jetting
            e(GCmd(g, "SH H"));
            e(GCmd(g, "ACH=20000000"));   // 200 mm/s^2
            e(GCmd(g, "JGH=1000"));   // 15 mm/s jog towards rear limit
            e(GCmd(g, "BGH"));

            e(GCmd(g, "SPX=5000"));

            curX = curX + table.data[setNum].lineLength.value;
            string xString = "PAX=" + to_string(75000 - ((50-curX)*1000));
            e(GCmd(g, xString.c_str()));
            e(GCmd(g, "BGX"));
            e(GMotionComplete(g, "X"));
            //disable jetting
            e(GCmd(g, "STH"));

            xString = "PAX=" + to_string(75000 - ((50-x_start)*1000));
            e(GCmd(g, xString.c_str()));
            curY = curY + table.data[setNum].lineSpacing.value;
            string yString = "PAY=" + to_string(curY*800); // This should be 800 for Y axis
            e(GCmd(g, yString.c_str()));
            e(GCmd(g, "BGX"));
            e(GCmd(g, "BGY"));
            e(GMotionComplete(g, "X"));
            e(GMotionComplete(g, "Y"));
            curX = x_start;
        }
    }
}

void progWindow::e(GReturn rc)
 {
   if (rc != G_NO_ERROR)
     throw rc;
 }

void progWindow::spread_x_layers(int num_layers) {//Spread Layers
    if(g){
        for(int i = 0; i < num_layers; ++i) {
            e(GCmd(g, "ACY=200000"));   // 200 mm/s^2
            e(GCmd(g, "DCY=200000"));   // 200 mm/s^2
            e(GCmd(g, "JGY=-25000"));   // 15 mm/s jog towards rear limit
            e(GCmd(g, "BGY"));
            e(GMotionComplete(g, "Y"));

            //SECTION 1
            //TURN ON HOPPER
            e(GCmd(g, "MG{P2} {^85}, {^49}, {^13}{N}"));

            // Slow move Jacob added that acted as a 'wait' for the hopper to fully turn on
            // look into GSleep(1000); command (I think this does what I want)
            e(GCmd(g, "SPY=10")); // 50 mm/s
            e(GCmd(g, "PRY=20")); //tune starting point
            e(GCmd(g, "BGY"));
            e(GMotionComplete(g, "Y"));

            e(GCmd(g, "SPY=40000")); // 50 mm/s
            e(GCmd(g, "PRY=92000")); //tune starting point
            e(GCmd(g, "BGY"));
            e(GMotionComplete(g, "Y"));

            //SECTION 2
            //TURN OFF HOPPER
            e(GCmd(g, "MG{P2} {^85}, {^48}, {^13}{N}"));
            e(GCmd(g, "SB 18"));    // Turns on rollers
            e(GCmd(g, "SB 21"));
            e(GCmd(g, "SPY=8000"));
            e(GCmd(g, "PRY=138000")); //tune starting point
            e(GCmd(g, "BGY"));
            e(GMotionComplete(g, "Y"));

            e(GCmd(g, "CB 18"));    // Turns off rollers
            e(GCmd(g, "CB 21"));
        }
    }
}

void progWindow::connectToController() {
    //TODO Maybe Threading or something like that? The gui is unresponsive until connect function is finished.
    e(GOpen(address, &g)); // Establish connection with motion controller
    e(GCmd(g, "SH XYZ")); // Enable X,Y, and Z motors

    // Controller Configuration
    e(GCmd(g, "MO")); // Ensure motors are off for setup

    // X Axis
    e(GCmd(g, "MTX = -1"));    // Set motor type to reversed brushless
    e(GCmd(g, "CEX = 2"));     // Set Encoder to reversed quadrature
    e(GCmd(g, "BMX = 40000")); // Set magnetic pitch of lienar motor
    e(GCmd(g, "AGX = 1"));     // Set amplifier gain
    e(GCmd(g, "AUX = 9"));     // Set current loop (based on inductance of motor)
    e(GCmd(g, "TLX = 3"));     // Set constant torque limit to 3V
    e(GCmd(g, "TKX = 0"));     // Disable peak torque setting for now

    // Y Axis
    e(GCmd(g, "MTY = 1"));     // Set motor type to standard brushless
    e(GCmd(g, "CEY = 0"));     // Set Encoder to reversed quadrature??? (or is it?)
    e(GCmd(g, "BMY = 2000"));  // Set magnetic pitch of rotary motor
    e(GCmd(g, "AGY = 1"));     // Set amplifier gain
    e(GCmd(g, "AUY = 11"));    // Set current loop (based on inductance of motor)
    e(GCmd(g, "TLY = 6"));     // Set constant torque limit to 6V
    e(GCmd(g, "TKY = 0"));     // Disable peak torque setting for now

    // Z Axis
    e(GCmd(g, "MTZ = -2.5"));  // Set motor type to standard brushless
    e(GCmd(g, "CEZ = 14"));    // Set Encoder to reversed quadrature
    e(GCmd(g, "AGZ = 0"));     // Set amplifier gain
    e(GCmd(g, "AUZ = 9"));     // Set current loop (based on inductance of motor)
    // Note: There might be more settings especially for this axis I might want to add later

    // H Axis (Jetting Axis)
    e(GCmd(g, "MTH = -2"));    // Set jetting axis to be stepper motor with defualt low
    e(GCmd(g, "AGH = 0"));     // Set gain to lowest value
    e(GCmd(g, "LDH = 3"));     // Disable limit sensors for H axis
    e(GCmd(g, "KSH = .25"));   // Minimize filters on step signals
    e(GCmd(g, "ITH = 1"));     // Minimize filters on step signals

    e(GCmd(g, "BN"));          // Save (burn) these settings to the controller just to be safe

    e(GCmd(g, "SH XYZ"));      // Enable X,Y, and Z motors
    e(GCmd(g, "CN= -1"));      // Set correct polarity for all limit switches

    //HOME ALL AXIS'
    if(g){
        // Home the X-Axis using the central home sensor index pulse
        e(GCmd(g, "ACX=200000"));   // 200 mm/s^2
        e(GCmd(g, "DCX=200000"));   // 200 mm/s^2
        e(GCmd(g, "JGX=-15000"));   // 15 mm/s jog towards rear limit
        e(GCmd(g, "ACY=200000"));   // 200 mm/s^2
        e(GCmd(g, "DCY=200000"));   // 200 mm/s^2
        e(GCmd(g, "JGY=25000"));   // 15 mm/s jog towards rear limit
        e(GCmd(g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
        e(GCmd(g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
        e(GCmd(g, "JGZ=-113664"));    // Speed of Z
        e(GCmd(g, "FLZ=2147483647")); // Turn off forward limit during homing
        try {
           e(GCmd(g, "BGX"));          // Start motion towards rear limit sensor
           } catch(...) {}
        try {
           e(GCmd(g, "BGY"));          // Start motion towards rear limit sensor
           } catch(...) {}
        try {
           e(GCmd(g, "BGZ")); // Start motion towards rear limit sensor
           } catch(...) {}
        try {
           //Temporary place for Jetting setup to save time
           jetter_setup();
        } catch(...) {}
        e(GMotionComplete(g, "X")); // Wait until limit is reached
        e(GMotionComplete(g, "Y")); // Wait until limit is reached
        e(GMotionComplete(g, "Z")); // Wait until limit is reached
        e(GCmd(g, "JGX=15000"));    // 15 mm/s jog towards home sensor
        e(GCmd(g, "HVX=500"));      // 0.5 mm/s on second move towards home sensor
        e(GCmd(g, "FIX"));          // Find index command for x axis
        e(GCmd(g, "ACY=50000")); // 62.5 mm/s^2
        e(GCmd(g, "DCY=50000")); // 62.5 mm/s^2
        e(GCmd(g, "SPY=25000")); // 31.25 mm/s
        e(GCmd(g, "PRY=-160000"));
        e(GCmd(g, "ACZ=757760"));
        e(GCmd(g, "DCZ=757760"));
        e(GCmd(g, "SDZ=1515520")); // Sets deceleration when limit switch is touched
        e(GCmd(g, "SPZ=113664"));
        e(GCmd(g, "PRZ=1025000"));//TUNE THIS BACKING OFF Z LIMIT TO FUTURE PRINT BED HEIGHT!
        e(GCmd(g, "BGX"));          // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
        e(GCmd(g, "BGY"));
        e(GCmd(g, "BGZ"));
        e(GMotionComplete(g, "X")); // Wait until X stage finishes moving
        e(GMotionComplete(g, "Y"));
        e(GMotionComplete(g, "Z")); // Wait until limit is reached
        e(GCmd(g, "PRX=-40000"));   //OFFSET TO ACCOUNT FOR THE SKEWED BINDER JET HEAD LOCATION
        e(GCmd(g, "BGX"));
        e(GMotionComplete(g, "X")); // Wait until X stage finishes moving
        e(GCmd(g, "DPX=75000"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
        e(GCmd(g, "DPY=0")); //Do we need this?
        e(GCmd(g, "DPZ=0"));
        e(GCmd(g, "FLZ=0")); // Set software limit on z so it can't go any higher than current position
         }

}
