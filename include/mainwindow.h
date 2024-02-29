#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <fstream>
#include "gmessagehandler.h"

class Printer;
class PrintThread;
class LinePrintWidget;
class OutputWindow;
class PowderSetupWidget;
class QMessageBox;
class BedMicroscopeWidget;

class JettingWidget;
class HighSpeedLineWidget;
class DropletObservationWidget;
namespace JetDrive { class Controller; }

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Printer *printer_, QMainWindow *parent = nullptr);
    ~MainWindow();
    void setup();

private slots:
    // TODO: change slot names away from using "on_" convention and connect slots manually in .cpp file
    void y_up_button_pressed();
    void x_right_button_pressed();
    void y_down_button_pressed();
    void x_left_button_pressed();

    void jog_released(); // executed when any jogging buttons above are released

    void on_xHome_clicked();
    void on_yHome_clicked();
    void on_zMax_clicked();
    void on_zUp_clicked();
    void on_zDown_clicked();
    void on_zMin_clicked();
    void on_connect_clicked();

    void on_activateRoller1_toggled(bool checked);
    void allow_user_input(bool allowed);
    void thread_ended();
    void connected_to_motion_controller();

    void print_to_output_window(QString s);
    void on_removeBuildBox_clicked();
    void on_actionShow_Hide_Console_triggered();
    void show_hide_droplet_analyzer_window();
    void generate_printing_message_box(const std::string &message);

    void tab_was_changed(int index);

    void stop_print_and_thread();

    void get_current_x_axis_position();
    void get_current_y_axis_position();
    void get_current_z_axis_position();

    void move_z_to_absolute_position();
    void open_log_file();

    void connect_to_jet_drive_button_pressed();
    void connect_to_pressure_controller_button_pressed();
    void connect_to_mister_button_pressed();
    void connect_motion_controller_button_pressed();

private:
    Ui::MainWindow *ui;
    void resizeEvent(QResizeEvent* event) override;

    Printer *printer {nullptr};

    LinePrintWidget *linePrintingWidget {nullptr};
    QDockWidget *dockWidget {nullptr};
    OutputWindow *outputWindow {nullptr};
    PowderSetupWidget *powderSetupWidget {nullptr};
    JettingWidget *jettingWidget {nullptr};
    HighSpeedLineWidget *highSpeedLineWidget {nullptr};
    DropletObservationWidget *dropletObservationWidget {nullptr};
    BedMicroscopeWidget *bedMicroscopeWidget {nullptr};

    QMessageBox *messageBox {nullptr};
    // TODO: should this go somewhere else?
    GMessageHandler *messageHandler {nullptr};


    std::ofstream logFile;
};
#endif // MAINWINDOW_H
