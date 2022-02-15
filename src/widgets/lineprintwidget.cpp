#include "lineprintwidget.h"
#include "ui_lineprintwidget.h"

#include <math.h>
#include <sstream>
#include <chrono>
#include <QDebug>

#include "printer.h"
#include "printhread.h"

using namespace std;

int jetter_setup();

LinePrintWidget::LinePrintWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::LinePrintWidget)
{
    ui->setupUi(this);
    setAccessibleName("Line Printing Widget");

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

LinePrintWidget::~LinePrintWidget()
{
    delete ui;
}

void LinePrintWidget::log(QString message, enum logType messageType = logType::Standard)
{
    // If current message type is an active log type
    if(std::find(activeLogTypes.begin(), activeLogTypes.end(), messageType) != activeLogTypes.end())
    {
        ui->consoleOutput->insertPlainText(message + "\n");
    }
}

void LinePrintWidget::updatePreviewWindow()
{
    std::vector<QLineF> lines = table.qLines(); // vector of lines to add to window
    ui->SVGViewer->scene()->clear(); // clear the window
    for(size_t i{0}; i < lines.size(); ++i) // for each line
    {
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
}

void LinePrintWidget::CheckCell(int row, int column)
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


void LinePrintWidget::updateCell(int row, int column)
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

void LinePrintWidget::updateTable(bool updateVerticalHeaders = false, bool updateHorizontalHeaders = false)
{
    // Set Table Size
    ui->tableWidget->setRowCount((int)table.data.size());
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

void LinePrintWidget::on_numSets_valueChanged(int rowCount)
{
    int prevRowCount = (int)table.data.size(); // get previous row count
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

void LinePrintWidget::on_tableWidget_cellChanged(int row, int column)
{
    CheckCell(row, column); // Check the cell that was changed
    updatePreviewWindow();
    ui->consoleOutput->ensureCursorVisible(); // Scroll to new content on console
}

void LinePrintWidget::on_startX_valueChanged(double arg1)
{
    table.startX = arg1;
    updatePreviewWindow();
}

void LinePrintWidget::on_startY_valueChanged(double arg1)
{
    table.startY = arg1;
    updatePreviewWindow();
}

void LinePrintWidget::on_setSpacing_valueChanged(double arg1)
{
    table.setSpacing = arg1;
    updatePreviewWindow();
}

void LinePrintWidget::on_printPercentSlider_sliderMoved(int position)
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

void LinePrintWidget::on_clearConsole_clicked()
{
    ui->consoleOutput->clear();
}

void LinePrintWidget::on_startPrint_clicked()
{
    std::stringstream s;

    auto t1{std::chrono::high_resolution_clock::now()};

    s << CMD::stop_motion(Axis::Jet);
    s << CMD::set_accleration(Axis::X, 200);
    s << CMD::set_deceleration(Axis::X, 200);
    s << CMD::set_accleration(Axis::Y, 200);
    s << CMD::set_deceleration(Axis::Y, 200);
    for(int i{0}; i < int(table.numRows()); ++i)
    {
        generate_line_set_commands(i, s); // Generate sets
    }

    auto t2{std::chrono::high_resolution_clock::now()};
    auto timeSpan = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    qDebug() << "This took:" << QString::number(timeSpan) << " milliseconds";

    emit disable_user_input();
    emit execute_command(s);
    emit generate_printing_message_box("Print is running.");
}

void LinePrintWidget::generate_line_set_commands(int setNum, std::stringstream &s)
{
    //Find starting position for line set
    float x_start{table.startX};
    for(int i{0}; i < setNum; ++i) // set proper x_start for set to be printed
    {
        x_start += table.data[i].lineLength.value + table.setSpacing;
    }
    float y_start{table.startY};

    s << CMD::set_speed(Axis::Y, 40);
    s << CMD::set_speed(Axis::X, 50);

    //calculate line specifics - TODO: CURRENTLY OVERCONSTRAINED

    //GO TO BEGINNING POINT

    // FIX THIS SECTION BY SETTING THE HOME POSITION OF THE STAGES TO ALWAYS BE THE SAME
    // AND THEN CREATE FUNCTIONS THAT GENERATE ABSOLUTE POSITIONS FROM BUILD PLATE COORDINATES

    // Maybe there is a better way of doing these commands than just a position absolute
    // coordinated motion with gearing for the jetting nozzle?

    // We also want the ability to raster in both directions...

    // the 50 is half the build plate size in the x-direction and the 75000 is the center position...

    s << CMD::position_absolute(Axis::X, 75 - (50 - x_start));
    s << CMD::position_absolute(Axis::Y, y_start);
    s << CMD::begin_motion(Axis::X);
    s << CMD::begin_motion(Axis::Y);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);

    // START LINE PRINTING HERE!
    float curY = y_start;
    float curX = x_start;
    for(int i{0}; i < table.data[setNum].numLines.value; ++i)
    {
        s << CMD::set_speed(Axis::X, table.data[setNum].printVelocity.value);

        // configure jetting
        s << CMD::servo_here(Axis::Jet);
        s << CMD::set_accleration(Axis::Jet, 20000000); // set super high acceleration for jetting axis
        s << CMD::set_jog(Axis::Jet, 1000);             // jetting frequency in hz

        curX = curX + table.data[setNum].lineLength.value;

        s << CMD::position_absolute(Axis::X, 75 - (50 - curX));
        s << CMD::begin_motion(Axis::X);
        s << CMD::begin_motion(Axis::Jet);
        s << CMD::motion_complete(Axis::X);
        s << CMD::stop_motion(Axis::Jet);

        s << CMD::set_speed(Axis::X, 50);                          // set x-axis move speed to 50 mm/s (change this to be user-settable in the future)
        s << CMD::position_absolute(Axis::X, 75 - (50 - x_start)); // PA to move x-axis to start of next line

        curY += table.data[setNum].lineSpacing.value;
        s << CMD::position_absolute(Axis::Y, curY);

        s << CMD::begin_motion(Axis::X);
        s << CMD::begin_motion(Axis::Y);
        s << CMD::motion_complete(Axis::X);
        s << CMD::motion_complete(Axis::Y);

        curX = x_start;
    }
}

void LinePrintWidget::allow_widget_input(bool allowed)
{
    if(allowed)
    {
        ui->startPrint->setEnabled(true);
    }
    else
    {
        ui->startPrint->setDisabled(true);
    }
}

