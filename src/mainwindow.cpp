#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <sstream>
#include <thread>
#include <QDockWidget>
#include <thread>

#include <QMessageBox>
#include <QProgressDialog>
#include <QDebug>

#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"
#include "jetdrive.h"
#include "printer.h"
#include "printhread.h"
#include "printerwidget.h"
#include "lineprintwidget.h"
#include "outputwindow.h"
#include "powdersetupwidget.h"
#include "jettingwidget.h"
#include "highspeedlinewidget.h"
#include "dropletobservationwidget.h"
#include "pcd.h"

MainWindow::MainWindow(Printer *printer_, QMainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    printer(printer_)
{
    ui->setupUi(this);

    // set up widgets (parent is set when adding as a tab)
    linePrintingWidget       = new LinePrintWidget(printer);
    powderSetupWidget        = new PowderSetupWidget(printer);
    highSpeedLineWidget      = new HighSpeedLineWidget(printer);
    dropletObservationWidget = new DropletObservationWidget(printer);

    // add widgets to tabs on the top bar (tab widget now owns)
    ui->tabWidget->addTab(powderSetupWidget, "Powder Setup");
    ui->tabWidget->addTab(linePrintingWidget, "Line Printing");
    ui->tabWidget->addTab(highSpeedLineWidget, "High-Speed Line Printing");
    ui->tabWidget->addTab(dropletObservationWidget, "Jetting");

    // set fill background for all tab widgets
    for (int i{0}; i < ui->tabWidget->count(); ++i)
    {
        auto widget = ui->tabWidget->widget(i);
        widget->setAutoFillBackground(true);
    }

    // open a txt file for logging commands
    open_log_file();
    logFile << "Application opened at "
            << QDateTime::currentDateTime()
               .toString("yyyy.MM.dd hh:mm")
               .toStdString()
            << "\n";

    // add dock widget for showing console output
    dockWidget = new QDockWidget("Output Window", this);
    this->addDockWidget(Qt::RightDockWidgetArea, dockWidget);
    outputWindow = new OutputWindow(this, logFile);
    dockWidget->setWidget(outputWindow);
    outputWindow->print_string("Starting Program...");
    outputWindow->print_string("Program Started");

    // disable all buttons that require a controller connection
    allow_user_input(false);


    // this really should be owned by the printer
    interruptHandler = new GInterruptHandler(this);

    connect(interruptHandler,
            &GInterruptHandler::status,
            this,
            [](uchar status){ qDebug() << QString::fromStdString(interrupt_string((Interrupt)status)); });

    // don't use for now since it doesn't always close right
    //interruptHandler->start();

}

// runs from main.cpp right after the object is initialized
void MainWindow::setup()
{

    // connect the output from the printer thread to the output window widget
    connect(printer->printerThread,
            &PrintThread::response,
            outputWindow,
            &OutputWindow::print_string);
    connect(printer->printerThread,
            &PrintThread::ended,
            this,
            &MainWindow::thread_ended);
    connect(printer->printerThread,
            &PrintThread::connected_to_controller,
            this,
            &MainWindow::connected_to_motion_controller);

    // connect signals for each of the printer widgets (this works recursively)
    QList<PrinterWidget*> printerWidgets = this->findChildren<PrinterWidget*>();
    for(int i{0}; i < printerWidgets.count(); ++i)
    {

        // connect signals and slots
        connect(printerWidgets[i],
                &PrinterWidget::execute_command,
                printer->printerThread,
                &PrintThread::execute_command);
        connect(printerWidgets[i],
                &PrinterWidget::generate_printing_message_box,
                this,
                &MainWindow::generate_printing_message_box);
        connect(printerWidgets[i],
                &PrinterWidget::disable_user_input,
                this,
                [this]() {this->allow_user_input(false);});
        connect(printerWidgets[i],
                &PrinterWidget::print_to_output_window,
                this,
                &MainWindow::print_to_output_window);
        connect(printerWidgets[i],
                &PrinterWidget::stop_print_and_thread,
                this,
                &MainWindow::stop_print_and_thread);

        connect(printerWidgets[i],
                &PrinterWidget::jet_turned_on,
                dropletObservationWidget,
                &DropletObservationWidget::jetting_was_turned_on);
        connect(printerWidgets[i],
                &PrinterWidget::jet_turned_off,
                dropletObservationWidget,
                &DropletObservationWidget::jetting_was_turned_off);
    }

    // connect jog buttons
    connect(ui->xRightButton, &QAbstractButton::pressed,
            this, &MainWindow::x_right_button_pressed);
    connect(ui->xLeftButton, &QAbstractButton::pressed,
            this, &MainWindow::x_left_button_pressed);
    connect(ui->yUpButton, &QAbstractButton::pressed,
            this, &MainWindow::y_up_button_pressed);
    connect(ui->yDownButton, &QAbstractButton::pressed,
            this, &MainWindow::y_down_button_pressed);

    connect(ui->xRightButton, &QAbstractButton::released,
            this, &MainWindow::jog_released);
    connect(ui->xLeftButton, &QAbstractButton::released,
            this, &MainWindow::jog_released);
    connect(ui->yUpButton, &QAbstractButton::released,
            this, &MainWindow::jog_released);
    connect(ui->yDownButton, &QAbstractButton::released,
            this, &MainWindow::jog_released);

    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::tab_was_changed);

    connect(ui->getXAxisPosition, &QAbstractButton::clicked,
            this, &MainWindow::get_current_x_axis_position);
    connect(ui->getYAxisPosition, &QAbstractButton::clicked,
            this, &MainWindow::get_current_y_axis_position);
    connect(ui->getZAxisPosition, &QAbstractButton::clicked,
            this, &MainWindow::get_current_z_axis_position);

    connect(ui->stopMotionButton, &QAbstractButton::clicked,
            this, [this]()
    {
        if (printer->g)
        {
            stop_print_and_thread();
            dropletObservationWidget->jetting_was_turned_off();
        }

    });

    connect(ui->zAbsoluteMoveButton, &QAbstractButton::clicked,
            this, &MainWindow::move_z_to_absolute_position);



    connect(ui->actionShow_Hide_Droplet_Tool, &QAction::triggered,
            this, &MainWindow::show_hide_droplet_analyzer_window);

    // connect response from jetDrive to output window
    connect(printer->jetDrive, &JetDrive::Controller::response, this,
            &MainWindow::print_to_output_window);
    connect(printer->jetDrive, &JetDrive::Controller::timeout, this,
            &MainWindow::print_to_output_window);
    connect(printer->jetDrive, &JetDrive::Controller::error, this,
            &MainWindow::print_to_output_window);

    connect(ui->connectToJetDriveButton, &QPushButton::pressed,
            this, &MainWindow::connect_to_jet_drive_button_pressed);


    // connect response from pressure controller to output window
    // TODO: Do I want these here?
    connect(printer->pressureController, &PCD::Controller::response, this, &MainWindow::print_to_output_window);
    connect(printer->pressureController, &PCD::Controller::timeout, this, &MainWindow::print_to_output_window);
    connect(printer->pressureController, &PCD::Controller::error, this, &MainWindow::print_to_output_window);

}

// on application close
MainWindow::~MainWindow()
{
    delete ui;
    if (printer->g) // if there is an active connection to a controller
    {
        GCmd(printer->g, "ST");
        GCmd(printer->g, "MO");    // Turn off the motors
        GCmd(printer->g, "CB 18"); // stop roller 1
        GCmd(printer->g, "CB 21"); // stop roller 2
        GCmd(printer->g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
        GClose(printer->g); // Close the connection to the controller
    }
    // log application close and close the file
    logFile << "Application closed at "
            << QDateTime::currentDateTime()
               .toString("yyyy.MM.dd hh:mm")
               .toStdString()
            << "\n";
    logFile.close();

    // I need to actually handle this
    interruptHandler->stop();
}

void MainWindow::on_connect_clicked()
{
    if (printer->g == 0) // if there is no connection to the motion controller
    {
        std::stringstream s;

        s << CMD::open_connection_to_controller();
        s << CMD::set_default_controller_settings();
        const bool homeZAxis = ui->homeZAxisCheckBox->isChecked();
        s << CMD::homing_sequence(homeZAxis);

        allow_user_input(false);
        printer->printerThread->execute_command(s);

        // Connect to JetDrive
        printer->jetDrive->connect_to_jet_drive();


        // this isn't ready yet
        //interruptHandler->connect_to_controller(printer->address);

    }
    else // if there is already a connection
    {
        allow_user_input(false);
        if (printer->g) // double check there is actually a connection
        {
            GCmd(printer->g, "ST"); // stop motion
            GCmd(printer->g, "MO"); // disable Motors
            GClose(printer->g);     // close connection to the motion controller
        }
        printer->g = 0;             // Reset connection handle

        printer->jetDrive->disconnect_serial();

        // change button label text
        ui->connect->setText("\nConnect to Controller\n");
        ui->homeZAxisCheckBox->setEnabled(true);
    }
}

void MainWindow::allow_user_input(bool allowed)
{
    ui->activateRoller1->setEnabled(allowed);
    ui->xHome->setEnabled(allowed);
    ui->yHome->setEnabled(allowed);
    ui->xRightButton->setEnabled(allowed);
    ui->yUpButton->setEnabled(allowed);
    ui->xLeftButton->setEnabled(allowed);
    ui->yDownButton->setEnabled(allowed);
    ui->zDown->setEnabled(allowed);
    ui->zUp->setEnabled(allowed);
    ui->zMax->setEnabled(allowed);
    ui->zMin->setEnabled(allowed);
    ui->removeBuildBox->setEnabled(allowed);

    ui->getXAxisPosition->setEnabled(allowed);
    ui->getYAxisPosition->setEnabled(allowed);
    ui->getZAxisPosition->setEnabled(allowed);
    ui->zAbsoluteMoveButton->setEnabled(allowed);

    // set if user can input on all printer widgets
    QList<PrinterWidget*> printerWidgets = this->findChildren<PrinterWidget*>();
    for(int i{0}; i < printerWidgets.count(); ++i)
    {
        printerWidgets[i]->allow_widget_input(allowed);
    }
}

void MainWindow::y_up_button_pressed()
{    
    std::stringstream s;

    s << CMD::set_accleration(Axis::Y, 300);
    s << CMD::set_deceleration(Axis::Y, 300);
    s << CMD::set_jog(Axis::Y, -ui->yVelocity->value());
    s << CMD::begin_motion(Axis::Y);

    printer->printerThread->execute_command(s);
}

void MainWindow::x_right_button_pressed()
{
    std::stringstream s;
    Axis x {Axis::X};

    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::set_jog(x, ui->xVelocity->value());
    s << CMD::begin_motion(x);

    printer->printerThread->execute_command(s);
}

void MainWindow::jog_released()
{
    std::stringstream s;
    s << CMD::stop_motion(Axis::X);
    s << CMD::stop_motion(Axis::Y);
    s << CMD::stop_motion(Axis::Z);
    s << CMD::motion_complete(Axis::X);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);

    printer->printerThread->execute_command(s);
}

void MainWindow::y_down_button_pressed()
{
    std::stringstream s;
    Axis y {Axis::Y};

    s << CMD::set_accleration(y, 300);
    s << CMD::set_deceleration(y, 300);
    s << CMD::set_jog(y, ui->yVelocity->value());
    s << CMD::begin_motion(y);

    // send off to thread to send to printer
    printer->printerThread->execute_command(s);
}

void MainWindow::x_left_button_pressed()
{
    std::stringstream s;
    Axis x {Axis::X};

    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::set_jog(x, -ui->xVelocity->value());
    s << CMD::begin_motion(x);

    printer->printerThread->execute_command(s);
}

void MainWindow::on_xHome_clicked()
{
    std::stringstream s;

    Axis x{Axis::X};
    s << CMD::set_accleration(x, 800);
    s << CMD::set_deceleration(x, 800);
    s << CMD::position_absolute(x, 0);
    s << CMD::set_speed(x, 50);
    s << CMD::begin_motion(x);
    s << CMD::motion_complete(x);

    allow_user_input(false);
    printer->printerThread->execute_command(s);
}

void MainWindow::on_yHome_clicked()
{
    std::stringstream s;
    Axis y {Axis::Y};

    s << CMD::set_accleration(y, 300);
    s << CMD::set_deceleration(y, 300);
    s << CMD::position_absolute(y, 0);
    s << CMD::set_speed(y, 50);
    s << CMD::begin_motion(y);
    s << CMD::motion_complete(y);

    allow_user_input(false);
    printer->printerThread->execute_command(s);
}

void MainWindow::on_zMax_clicked()
{     
    std::stringstream s;
    Axis z {Axis::Z};

    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_jog(z, 1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    allow_user_input(false);
    printer->printerThread->execute_command(s);
}

void  MainWindow::on_zUp_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};
    //                                              convert from microns
    s << CMD::position_relative(z, ui->zStepSize->value() / 1000.0);
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);


    allow_user_input(false);
    printer->printerThread->execute_command(s);
}

void  MainWindow::on_zDown_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};
    //                                              convert from microns
    s << CMD::position_relative(z, -ui->zStepSize->value() / 1000.0);
    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_speed(z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    allow_user_input(false);
    printer->printerThread->execute_command(s);
}

void  MainWindow::on_zMin_clicked()
{
    std::stringstream s;
    Axis z {Axis::Z};

    s << CMD::set_accleration(z, 10);
    s << CMD::set_deceleration(z, 10);
    s << CMD::set_jog(z, -1.5);
    s << CMD::begin_motion(z);
    s << CMD::motion_complete(z);

    allow_user_input(false);
    printer->printerThread->execute_command(s);
}

void MainWindow::on_activateRoller1_toggled(bool checked)
{
    std::stringstream s;

    if (checked == 1) s << CMD::enable_roller1();
    else              s << CMD::disable_roller1();

    printer->printerThread->execute_command(s);
}

void MainWindow::print_to_output_window(QString s)
{
    outputWindow->print_string(s);
}

void MainWindow::on_removeBuildBox_clicked()
{
    std::stringstream s;

    int yAxisAcceleration{50};
    int yAxisJogVelocity{30};
    int zAxisAcceleration{20};
    int zAxisJogVelocity{-2};
    int sleepTimeMilliseconds{500};
    double zAxisOffsetFromLowerLimit{2};
    int zAxisOffsetMoveSpeed{1};

    s << CMD::set_accleration(Axis::Y, yAxisAcceleration);
    s << CMD::set_deceleration(Axis::Y, yAxisAcceleration);
    s << CMD::set_jog(Axis::Y, yAxisJogVelocity); // jog to front

    s << CMD::set_accleration(Axis::Z, zAxisAcceleration);
    s << CMD::set_deceleration(Axis::Z, zAxisAcceleration);
    s << CMD::set_jog(Axis::Z, zAxisJogVelocity); // jog to bottom

    s << CMD::begin_motion(Axis::Y);
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Y);
    s << CMD::motion_complete(Axis::Z);

    s << CMD::sleep(sleepTimeMilliseconds);
    s << CMD::position_relative(Axis::Z, zAxisOffsetFromLowerLimit);
    s << CMD::set_speed(Axis::Z, zAxisOffsetMoveSpeed);

    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);

    allow_user_input(false);
    printer->printerThread->execute_command(s);
}


void MainWindow::on_actionShow_Hide_Console_triggered()
{
    if (dockWidget->isVisible()) dockWidget->hide();
    else                         dockWidget->show();
}

void MainWindow::show_hide_droplet_analyzer_window()
{
    if (!dropletObservationWidget->is_droplet_anlyzer_window_visible())
        dropletObservationWidget->show_droplet_analyzer_widget();
    else
        dropletObservationWidget->hide_droplet_analyzer_widget();
}

void MainWindow::generate_printing_message_box(const std::string &message)
{
    // This could be a way to do this in the future...
    //QProgressDialog *progressDialog = new QProgressDialog(this);
    //progressDialog->setLabelText("testing");
    //progressDialog->open();

    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(message));
    msgBox.setInformativeText("Click cancel to stop");
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    switch (ret)
    {
    case QMessageBox::Cancel: stop_print_and_thread(); break;
    default: break;
    }
}

void MainWindow::stop_print_and_thread()
{
    printer->printerThread->stop();
    if (printer->g)
    {
        // Can't use CMD:: commands here...
        // work on a way to either send these through the thread
        // even though it is blocked or be able to strip CMD:: commands
        // of new line and beginning command type so I can use them here
        GCmd(printer->g, "HX");    // Halt any running program
        GCmd(printer->g, "ST");    // stop motion on all axes
        GCmd(printer->g, "CB 18"); // stop roller 1
        GCmd(printer->g, "CB 21"); // stop roller 2
        GCmd(printer->g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
    }
}

void MainWindow::get_current_x_axis_position()
{
    if (printer->g)
    {
        int currentXPos;
        GCmdI(printer->g, "TPX", &currentXPos);
        double currentXPos_mm = currentXPos / (double)X_CNTS_PER_MM;
        print_to_output_window("Current X: "
                               + QString::number(currentXPos_mm)
                               + "mm");
    }
}

void MainWindow::get_current_y_axis_position()
{
    if (printer->g)
    {
        int currentYPos;
        GCmdI(printer->g, "TPY", &currentYPos);
        double currentYPos_mm = currentYPos / (double)Y_CNTS_PER_MM;
        print_to_output_window("Current Y: "
                               + QString::number(currentYPos_mm)
                               + "mm");
    }
}

void MainWindow::get_current_z_axis_position()
{
    if (printer->g)
    {
        int currentZPos;
        GCmdI(printer->g, "TPZ", &currentZPos);
        double currentZPos_mm = currentZPos / (double)Z_CNTS_PER_MM;
        print_to_output_window("Current Z: "
                               + QString::number(currentZPos_mm)
                               + "mm");
    }
}

void MainWindow::move_z_to_absolute_position()
{
    std::stringstream s;

    s << CMD::position_absolute(Axis::Z, ui->zAbsoluteMoveSpinBox->value());
    s << CMD::set_accleration(Axis::Z, 10);
    s << CMD::set_deceleration(Axis::Z, 10);
    s << CMD::set_speed(Axis::Z, 1.5); // max speed of 5 mm/s!
    s << CMD::begin_motion(Axis::Z);
    s << CMD::motion_complete(Axis::Z);

    allow_user_input(false);
    printer->printerThread->execute_command(s);
}

void MainWindow::open_log_file()
{
    QString folderName = "BJ_Logs";
    QString logDir =
            QStandardPaths::writableLocation
            (QStandardPaths::DocumentsLocation)
            + "/"
            + folderName;
    if(!QDir(logDir).exists()) QDir().mkdir(logDir);
    QDateTime date = QDateTime::currentDateTime();
    QString fileName = logDir
            + "/BJ_Log_"
            + date.toString("yyyy_MM_dd")
            + ".txt";
    logFile.open(fileName.toStdString(), std::ios::app); // append
}

void MainWindow::connect_to_jet_drive_button_pressed()
{
    printer->jetDrive->connect_to_jet_drive();
}

void MainWindow::thread_ended()
{
    allow_user_input(true);
}

// override the resize event of the main window
void MainWindow::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);
   highSpeedLineWidget->reset_preview_zoom();
}

// code that gets run when the current tab in the mainwindow is changed
void MainWindow::tab_was_changed(int index)
{
    if (index == ui->tabWidget->indexOf(highSpeedLineWidget))
    {
        highSpeedLineWidget->reset_preview_zoom();
    }
}

void MainWindow::connected_to_motion_controller()
{
    ui->connect->setText("\nDisconnect Controller\n");
    ui->homeZAxisCheckBox->setEnabled(false);
}

#include "moc_mainwindow.cpp"
