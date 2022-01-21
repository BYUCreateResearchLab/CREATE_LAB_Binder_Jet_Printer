#include "progwindow.h"
#include "ui_progwindow.h"

#include <math.h>
#include <iostream>
#include <sstream>

#include "printer.h"
#include "printhread.h"
#include "commandcodes.h"

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

void progWindow::setup(Printer *printerPtr, PrintThread *printerThread, OutputWindow *outputWindow)
{
    printer = printerPtr;
    mPrintThread = printerThread;
    mOutputWindow = outputWindow;
}

void progWindow::log(QString message, enum logType messageType = logType::Standard)
{
    // If current message type is an active log type
    if(std::find(activeLogTypes.begin(), activeLogTypes.end(), messageType) != activeLogTypes.end())
    {
        ui->consoleOutput->insertPlainText(message + "\n");
    }
}

void progWindow::updatePreviewWindow()
{
    std::vector<QLineF> lines = table.qLines(); // vector of lines to add to window
    ui->SVGViewer->scene()->clear(); // clear the window
    for(size_t i{0}; i < lines.size(); ++i) // for each line
    {
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
}

void progWindow::CheckCell(int row, int column)
{
    QString cellText = ui->tableWidget->item(row, column)->text();

    if(cellText == table.data[row][column].toQString())
    {
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
    if(rowCount > prevRowCount) // If adding rows
    {
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

    for(int i{0}; i < numLinestoShow; ++i) // for each line
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
        std::stringstream s;
        for(int i{0}; i < num_layers; ++i)
        {
            s << GCmd() << "ACY=" << mm2cnts(200, 'Y')            << "\n";
            s << GCmd() << "DCY=" << mm2cnts(200, 'Y')            << "\n";
            s << GCmd() << "JGY=" << mm2cnts(-30, 'Y')            << "\n"; // jog y to back
            s << GCmd() << "BGY"                                  << "\n";
            s << GMotionComplete() << "Y"                         << "\n";

            // Set hopper intensity
            // set the ultrasonic generator mode to 0 (A) and the intensity to 5 (50%) "M05"
            s << GCmd() << "MG{P2} {^77}, {^48}, {^53}, {^13}{N}" << "\n"; // Set intensity
            s << GCmd() << "MG{P2} {^85}, {^49}, {^13}{N}"        << "\n"; // turn off hopper
            s << GSleep() << 1000                                 << "\n"; // wait 1000 ms
            s << GCmd() << "SPY="  << mm2cnts(50, 'Y')            << "\n"; // hopper traversal speed
            s << GCmd() << "PRY="  << mm2cnts(115, 'Y')           << "\n"; // move y axis under hopper a specified distance
            s << GCmd() << "BGY"                                  << "\n"; // begin motion
            s << GMotionComplete() << "Y"                         << "\n"; // pause until motion complete

            s << GCmd() << "MG{P2} {^85}, {^48}, {^13}{N}"        << "\n"; // turn off hopper
            s << GCmd() << "SB "   << ROLLER_1_BIT                << "\n"; // turn on roller 1
            s << GCmd() << "SB "   << ROLLER_2_BIT                << "\n"; // turn on roller 2
            s << GCmd() << "SPY="  << mm2cnts(10, 'Y')            << "\n";
            s << GCmd() << "PRY="  << mm2cnts(172.5, 'Y')         << "\n"; // Distance to move under roller (CONSIDER CHANGING TO ABSOLUTE POSITION IN THE FUTURE)
            s << GCmd() << "BGY"                                  << "\n";
            s << GMotionComplete() << "Y"                         << "\n";

            s << GCmd() << "CB " << ROLLER_1_BIT                  << "\n";  // turn off roller 1
            s << GCmd() << "CB " << ROLLER_2_BIT                  << "\n";  // turn off roller 2
        }

        mPrintThread->execute_command(s); // roll all layers
    }
}

void progWindow::on_levelRecoat_clicked()
{
    emit printing_from_prog_window();
    spread_x_layers(ui->layersToSpread->value());
}

void progWindow::on_startPrint_clicked()
{
    std::stringstream s;
    for(int i{0}; i < int(table.numRows()); ++i)
    {
        generate_line_set_commands(i, s); // Generate sets
    }

    emit printing_from_prog_window();
    mPrintThread->execute_command(s);
}

void progWindow::generate_line_set_commands(int setNum, std::stringstream &s)
{
    //Find starting position for line set
    float x_start{table.startX};
    for(int i{0}; i < setNum; ++i) // set proper x_start for set to be printed
    {
        x_start += table.data[i].lineLength.value + table.setSpacing;
    }
    float y_start{table.startY};

    s << GCmd() << "SPY=" << mm2cnts(30, 'Y') << "\n"; // set y-axis speed

    //calculate line specifics - TODO: CURRENTLY OVERCONSTRAINED

    //GO TO BEGINNING POINT

    // FIX THIS SECTION BY SETTING THE HOME POSITION OF THE STAGES TO ALWAYS BE THE SAME
    // AND THEN CREATE FUNCTIONS THAT GENERATE ABSOLUTE POSITIONS FROM BUILD PLATE COORDINATES

    // Maybe there is a better way of doing these commands than just a position absolute
    // coordinated motion with gearing for the jetting nozzle?

    // We also want the ability to raster in both directions...

    // the 50 is half the build plate size in the x-direction and the 75000 is the center position...

    s << GCmd() << "PAX="  << 75000 - ((50-x_start) * 1000)           << "\n";
    s << GCmd() << "PAY="  << y_start * 800                           << "\n";
    s << GCmd() << "BGX"                                              << "\n";
    s << GCmd() << "BGY"                                              << "\n";
    s << GMotionComplete() << "X"                                     << "\n";
    s << GMotionComplete() << "Y"                                     << "\n";

    // START LINE PRINTING HERE!
    float curY = y_start;
    float curX = x_start;
    for(int i{0}; i < table.data[setNum].numLines.value; ++i)
    {
        s << GCmd() << "SPX=" << mm2cnts(table.data[setNum].printVelocity.value, 'X') << "\n"; // set x-axis print speed

        // configure jetting
        s << GCmd() << "SH H"                                         << "\n"; // enable jetting axis
        s << GCmd() << "ACH=20000000"                                 << "\n"; // set super high acceleration for jetting axis
        s << GCmd() << "JGH=" << table.data[setNum].jettingFreq.value << "\n"; // set jetting frequency

        curX = curX + table.data[setNum].lineLength.value;

        s << GCmd() << "PAX="  << 75000 - ((50-curX) * 1000)          << "\n";
        s << GCmd() << "BGX"                                          << "\n"; // begin x-axis move
        s << GCmd() << "BGH"                                          << "\n"; // begin jetting
        s << GMotionComplete() << "X"                                 << "\n"; // wait until x-axis is done
        s << GCmd() << "STH"                                          << "\n"; // disable jetting

        s << GCmd() << "SPX=" << mm2cnts(50, 'X')                     << "\n"; // set x-axis move speed to 50 mm/s (change this to be user-settable in the future)
        s << GCmd() << "PAX=" << 75000 - ((50-x_start) * 1000)        << "\n"; // PA to move x-axis to start of next line

        curY += table.data[setNum].lineSpacing.value;
        s << GCmd() << "PAY=" << mm2cnts(curY, 'Y')                   << "\n"; // PA to move y-axis to start of next line

        s << GCmd() << "BGX"                                          << "\n"; // move x-axis to start of next line
        s << GCmd() << "BGY"                                          << "\n"; // move y-axis to start of next line
        s << GMotionComplete() << "X"                                 << "\n";
        s << GMotionComplete() << "Y"                                 << "\n"; // wait until both moves are complete

        curX = x_start;
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

