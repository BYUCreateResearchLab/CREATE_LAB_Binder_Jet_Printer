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
    //ui->startX->setValue(table.startX);
    //ui->startY->setValue(table.startY);
    //ui->setSpacing->setValue(table.setSpacing);

    // get values from ui and update internal data structure
    table.startX = ui->startX->value();
    table.startY = ui->startY->value();
    table.setSpacing = ui->setSpacing->value();


    //SVG Viewer setup
    ui->SVGViewer->setup(PRINT_X_SIZE_MM, PRINT_Y_SIZE_MM);
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
    std::vector<QLineF> accelerationLines = table.qAccelerationLines();
    //ui->SVGViewer->scene()->clear(); // clear the window
    ui->SVGViewer->clear_lines();
    for(size_t i{0}; i < lines.size(); ++i) // for each line
    {
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
    for(size_t i{0}; i < accelerationLines.size(); ++i) // for each line
    {
        ui->SVGViewer->scene()->addLine(accelerationLines[i], lineTravelPen); // add the line to the scene
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
    switch(error_state)
    {
    case errorNone: // no error
        log("The input was successful and read as " +
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

    int dropletSpacingColumn = table.get_column_index_for(LineSet().dropletSpacing);
    int jettingFreqColumn = table.get_column_index_for(LineSet().jettingFreq);
    int printVelocityColumn = table.get_column_index_for(LineSet().printVelocity);

    if (column == dropletSpacingColumn || column == jettingFreqColumn)
    {
        double newPrintSpeed = (table.data[row].dropletSpacing.value * table.data[row].jettingFreq.value)/1000.0;
        ui->tableWidget->item(row, printVelocityColumn)->setText(QString::number(newPrintSpeed));
    }

    // Update cell text for user interface
    ui->tableWidget->item(row, column)->setText(table.data[row][column].toQString());

    // move start x position if the first set is too close to edge to have space to accelerate
    if (row == 0)
    {
        check_x_start();
    }
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

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    disable_velocity_input();
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
    check_x_start();
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
    ui->SVGViewer->clear_lines(); // clear the window

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

    s << CMD::stop_motion(Axis::Jet); // stop jetting if it is currently jetting

    //s << CMD::set_accleration(Axis::X, 500);
    //s << CMD::set_deceleration(Axis::X, 500);
    // x axis acceleration is set by the user now

    s << CMD::set_accleration(Axis::Y, 300);
    s << CMD::set_deceleration(Axis::Y, 300);

    for(int i{0}; i < int(table.numRows()); ++i)
    {
        std::string setMessage = "Set " + std::to_string(i+1) + " of " + std::to_string(table.numRows());
        s << CMD::display_message(setMessage);
        generate_line_set_commands(i, s); // Generate sets
    }

    s << CMD::display_message("Print Complete");

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
    float lineStartX = table.startX;
    for (int i{0}; i < setNum; ++i) // set proper x_start for current set to be printed
    {
        lineStartX += table.data[i].lineLength.value + table.setSpacing;
    }
    LineSet *currentLineSet = &table.data[setNum];

    s << CMD::set_accleration(Axis::Y, 400);
    s << CMD::set_deceleration(Axis::Y, 400);
    s << CMD::set_accleration(Axis::X, currentLineSet->printAcceleration.value);
    s << CMD::set_deceleration(Axis::X, currentLineSet->printAcceleration.value);

    // maybe make an pre-offset position absolute command

    // move to the left of the first line to be printed
    //     offset by the distance it will take to accelerate

    double accelerationDistance = calculate_acceleration_distance(currentLineSet->printVelocity.value, currentLineSet->printAcceleration.value);
    int accelerationTime_ms = (int)((currentLineSet->printVelocity.value/currentLineSet->printAcceleration.value) * 1000.0);
    int printTime_ms = (int)((currentLineSet->lineLength.value/currentLineSet->printVelocity.value) * 1000.0);

    lineStartX += (Printer2NozzleOffsetX - accelerationDistance);
    double lineEndX = lineStartX + currentLineSet->lineLength.value + (2.0*accelerationDistance);
    double lineYPos = table.startY + Printer2NozzleOffsetY;

    // configure jetting
    s << CMD::servo_here(Axis::Jet);
    s << CMD::set_accleration(Axis::Jet, 20000000); // set super high acceleration for jetting axis
    //s << CMD::set_jog(Axis::Jet, 1000);             // jetting frequency in hz

    // START LINE PRINTING HERE!
    for (int i{0}; i < currentLineSet->numLines.value; ++i)
    {
        s << CMD::set_speed(Axis::Y, 60);
        s << CMD::set_speed(Axis::X, 80);

        s << CMD::position_absolute(Axis::X, lineStartX);
        s << CMD::position_absolute(Axis::Y, lineYPos);
        s << CMD::begin_motion(Axis::X);
        s << CMD::begin_motion(Axis::Y);
        s << CMD::motion_complete(Axis::X);
        s << CMD::motion_complete(Axis::Y);

        s << CMD::set_speed(Axis::X, currentLineSet->printVelocity.value);

        // Set H to gear as a slave of X
        s << CMD::enable_gearing_for(Axis::Jet, Axis::X);
        std::string lineMessage = "Printing line " + std::to_string(i+1) + " of " + std::to_string((int)currentLineSet->numLines.value);
        s << CMD::display_message(lineMessage);
        s << CMD::position_absolute(Axis::X, lineEndX);
        s << CMD::begin_motion(Axis::X);

        s << CMD::sleep(accelerationTime_ms);
        s << CMD::set_jetting_gearing_ratio_from_droplet_spacing(Axis::X, currentLineSet->dropletSpacing.value);
        // Once the x-axis reaches the start of the line, enable gearing and start jetting
        //s << CMD::after_absolute_position(Axis::X, lineStartX + accelerationDistance);

        // After the line is printed turn disable gearing to stop jetting
        //s << CMD::after_absolute_position(Axis::X, lineStartX + accelerationDistance + currentLineSet->lineLength.value);
        s << CMD::sleep(printTime_ms);
        s << CMD::disable_gearing_for(Axis::Jet);

        s << CMD::motion_complete(Axis::X);
        //s << CMD::stop_motion(Axis::Jet);

        // move to start of next line
        lineYPos += currentLineSet->lineSpacing.value;   // move y by the line spacing amount
    }
}

void LinePrintWidget::allow_widget_input(bool allowed)
{
    if (allowed)
    {
        ui->startPrint->setEnabled(true);
    }
    else
    {
        ui->startPrint->setDisabled(true);
    }
}

void LinePrintWidget::disable_velocity_input()
{
    int column = table.get_column_index_for(LineSet().printVelocity);
    for (int row=0; row < ui->tableWidget->model()->rowCount(); row++)
    {
        auto currentFlags = ui->tableWidget->item(row, column)->flags();
        ui->tableWidget->item(row, column)->setFlags(currentFlags & (~Qt::ItemIsEnabled));
    }
}

void LinePrintWidget::check_x_start()
{
    int row = 0;
    double accelerationDistance = calculate_acceleration_distance(table.data[row].printVelocity.value, table.data[row].printAcceleration.value);
    double xStartPos = table.startX + (Printer2NozzleOffsetX - accelerationDistance);
    if (xStartPos < 0)
    {
        table.startX -= xStartPos;
        ui->startX->setValue(table.startX);
        log("The x start position had to be changed to " +
            QString::number(table.startX) + " to accomodate acceleration before printing", logType::Error);
    }
}

