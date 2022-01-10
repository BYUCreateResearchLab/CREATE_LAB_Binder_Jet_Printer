#include "progwindow.h"
#include "ui_progwindow.h"
//#include "JetServer.h"
#include <math.h>
#include <iostream>

using namespace std;

int jetter_setup();

progWindow::progWindow(QWidget *parent) : QWidget(parent), ui(new Ui::progWindow)
{
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

void progWindow::setup(Printer *printerPtr)
{
    printer = printerPtr;
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
    for(size_t i=0; i < lines.size(); i++) // for each line
    {
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
    if(!item || item->text().isEmpty())
    {
        ui->tableWidget->setItem(row,column, new QTableWidgetItem(table.data[row][column].toQString()));
    }
    else
    {
        item->setText(table.data[row][column].toQString());
    }
}

void progWindow::updateTable(bool updateVerticalHeaders = false, bool updateHorizontalHeaders = false)
{
    // Set Table Size
    ui->tableWidget->setRowCount(table.data.size());
    ui->tableWidget->setColumnCount(table.data[0].size);

    // Update Vertical Headers if updateVerticalHeaders is set to true
    if(updateVerticalHeaders)
    {
        QStringList verticalHeaders = QStringList(); // Create new string list for setting vertical headers
        for(int i = 0; i < ui->tableWidget->rowCount(); i++) // For each row
        {
            verticalHeaders.append(QString::fromStdString("Set " + std::to_string(i + 1))); // Add string to list
        }
        ui->tableWidget->setVerticalHeaderLabels(verticalHeaders);
    }

    // Update Horizontal Headers if updateHorizontalHeaders is set to true
    if(updateHorizontalHeaders)
    {
        QStringList horizontalHeaders = QStringList(); // Create new string list for setting vertical headers
        for(int i = 0; i < ui->tableWidget->columnCount(); i++) // For each row
        {
            horizontalHeaders.append(QString::fromStdString(table.data[0][i].typeName)); // Add string to list
        }
        ui->tableWidget->setHorizontalHeaderLabels(horizontalHeaders);
        ui->tableWidget->horizontalHeader()->setVisible(true);
    }

    // Replace cell text for each cell in UI Table
    for(int r = 0; r < ui->tableWidget->rowCount(); r++)
    {
        for(int c = 0; c < ui->tableWidget->columnCount(); c++)
        {
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
    if(rowCount > prevRowCount)
    { // If adding rows
        table.addRows(rowCount - prevRowCount);
        updateTable(true, false);
        updatePreviewWindow();
    }
    if(rowCount < prevRowCount) // If removing rows
    {
        table.removeRows(prevRowCount - rowCount);
        updateTable(true, false);
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

    for(int i=0; i < numLinestoShow; i++) // for each line
    {
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
}

void progWindow::on_clearConsole_clicked()
{
    ui->consoleOutput->clear();
}

void progWindow::spread_x_layers(int num_layers)
{
    if(printer->g)
    {
        for(int i{0}; i < num_layers; ++i)
        {
            e(GCmd(printer->g, "ACY=200000"));   // 250 mm/s^2
            e(GCmd(printer->g, "DCY=200000"));   // 250 mm/s^2
            e(GCmd(printer->g, "JGY=-25000"));   // 31.25 mm/s jog towards rear limit
            e(GCmd(printer->g, "BGY"));
            e(GMotionComplete(printer->g, "Y"));

            // Set hopper intensity
            // set the ultrasonic generator mode to 0 (A) and the intensity to 5 (50%) "M05"
            e(GCmd(printer->g, "MG{P2} {^77}, {^48}, {^53}, {^13}{N}"));

            //SECTION 1
            e(GCmd(printer->g, "MG{P2} {^85}, {^49}, {^13}{N}")); //TURN ON HOPPER

            // Slow move Jacob added that acted as a 'wait' for the hopper to fully turn on
            // look into GSleep(1000); command (I think this does what I want)
            e(GCmd(printer->g, "SPY=10"));
            e(GCmd(printer->g, "PRY=20"));
            e(GCmd(printer->g, "BGY"));
            e(GMotionComplete(printer->g, "Y"));

            e(GCmd(printer->g, "SPY=40000")); // 50 mm/s
            e(GCmd(printer->g, "PRY=92000")); //tune starting point
            e(GCmd(printer->g, "BGY"));
            e(GMotionComplete(printer->g, "Y"));

            //SECTION 2
            //TURN OFF HOPPER
            e(GCmd(printer->g, "MG{P2} {^85}, {^48}, {^13}{N}"));
            // turn on rollers
            e(GCmd(printer->g, "SB 18"));    // Turn on roller 1
            e(GCmd(printer->g, "SB 21"));    // Turn on roller 2
            e(GCmd(printer->g, "SPY=8000"));
            e(GCmd(printer->g, "PRY=138000")); //tune starting point
            e(GCmd(printer->g, "BGY"));
            e(GMotionComplete(printer->g, "Y"));

            // Turn off rollers
            e(GCmd(printer->g, "CB 18"));    // Turn off roller 1
            e(GCmd(printer->g, "CB 21"));    // Turn off roller 2
        }
    }
}

void progWindow::on_levelRecoat_clicked()
{
   spread_x_layers(ui->layersToSpread->value());
}

void progWindow::on_startPrint_clicked()
{
    //GO TO XSTART, YSTART, ZMAX
    if(printer->g == 0)
    {
        // How to handle if the controller has not been connected?
    }

    else
    {
        //Print Sets
        for(int i{0}; i < int(table.numRows()); ++i)
        {
            printLineSet(i);
        }

        /*
        //GO TO HOME AFTER PRINT
        string xHomeString = "PAX=" + to_string(75000 - ((50-table.startX)*1000));
        e(GCmd(printer->g, xHomeString.c_str()));
        string yHomeString = "PAY=" + to_string(table.startY*800);
        e(GCmd(printer->g, yHomeString.c_str()));
        e(GCmd(printer->g, "BGX"));
        e(GCmd(printer->g, "BGY"));
        e(GMotionComplete(printer->g, "X")); // Wait until limit is reached
        e(GMotionComplete(printer->g, "Y"));
        */
    }
}

void progWindow::printLineSet(int setNum)
{
    //Find starting position for line set
    float x_start{table.startX};
    for(int i{0}; i < setNum; ++i) // set proper x_start for set to be printed
    {
        x_start += table.data[i].lineLength.value + table.setSpacing;
    }
    float y_start{table.startY};

    e(GCmd(printer->g, "SPY=25000")); // 31.25 mm/s (Put this here for now, but we will want a setting for y step speed during line printing)

    //calculate line specifics - TODO: CURRENTLY OVERCONSTRAINED

    if(printer->g)
    {
        //GO TO BEGINNING POINT
        string xHomeString = "PAX=" + to_string(75000 - ((50-x_start) * 1000)); //why is this 50 here? It's also down below...
        e(GCmd(printer->g, xHomeString.c_str()));
        string yHomeString = "PAY=" + to_string(y_start * 800);
        e(GCmd(printer->g, yHomeString.c_str()));
        e(GCmd(printer->g, "BGX"));
        e(GCmd(printer->g, "BGY"));
        e(GMotionComplete(printer->g, "X")); // Wait until limit is reached
        e(GMotionComplete(printer->g, "Y"));

        //START LINE PRINTING HERE!
        float curY = y_start;
        float curX = x_start;
        for(int i{0}; i < table.data[setNum].numLines.value; ++i)
        {
            // set x-axis print speed
            string xPrintSpeedString = "SPX=" + to_string(table.data[setNum].printVelocity.value * 1000.0f);
            e(GCmd(printer->g, xPrintSpeedString.c_str()));

            //configure jetting
            e(GCmd(printer->g, "SH H")); // enable jetting axis
            e(GCmd(printer->g, "ACH=20000000")); // set super high acceleration
            // set jetting frequency
            string jettingFreq = "JGH=" + to_string(table.data[setNum].jettingFreq.value);
            e(GCmd(printer->g, jettingFreq.c_str()));   // set jetting frequency

            curX = curX + table.data[setNum].lineLength.value;
            string xString = "PAX=" + to_string(75000 - ((50-curX) * 1000));
            e(GCmd(printer->g, xString.c_str()));
            e(GCmd(printer->g, "BGX")); // begin x-axis move
            e(GCmd(printer->g, "BGH")); // begin jetting
            e(GMotionComplete(printer->g, "X")); // wait until move is complete
            e(GCmd(printer->g, "STH")); //disable jetting

            e(GCmd(printer->g, "SPX=50000")); // set x-axis move speed to 50 mm/s (change this to be user-settable in the future)
            xString = "PAX=" + to_string(75000 - ((50-x_start) * 1000));
            e(GCmd(printer->g, xString.c_str())); // PA to move x-axis to start of next line
            curY = curY + table.data[setNum].lineSpacing.value;
            string yString = "PAY=" + to_string(curY * 800);
            e(GCmd(printer->g, yString.c_str())); // PA to move y-axis to start of next line
            e(GCmd(printer->g, "BGX")); // move x-axis to start of next line
            e(GCmd(printer->g, "BGY")); // move y-axis to start of next line
            e(GMotionComplete(printer->g, "X"));
            e(GMotionComplete(printer->g, "Y")); // wait until both are complete
            curX = x_start;
        }
    }
}

// WORK ON UPDATING THIS TO OUTPUT TO OUTPUT WINDOW
void progWindow::e(GReturn rc)
{
    if (rc != G_NO_ERROR)
        throw rc;
}

void progWindow::set_connected(bool isConnected)
{
    if(isConnected)
    {
        ui->levelRecoat->setEnabled(true);
        ui->startPrint->setEnabled(true);
    }
    else
    {
        ui->levelRecoat->setDisabled(true);
        ui->startPrint->setDisabled(true);
    }
}

