#include "progwindow.h"
#include "ui_progwindow.h"

progWindow::progWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::progWindow)
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

