#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Printer;
class PrintThread;
class LinePrintWidget;
class OutputWindow;
class PowderSetupWidget;

class JettingWidget;
class HighSpeedLineWidget;
class DropletObservationWidget;
class JetDrive;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QMainWindow *parent = nullptr);
    ~MainWindow();
    void setup(Printer *printerPtr, PrintThread *printerThread);

private slots:
    // TODO: change slot names away from using "on_" convention and connect slots manually in .cpp file
    void on_yPositive_pressed(); // used when +y button is pressed
    void on_xPositive_pressed();
    void on_yNegative_pressed();
    void on_xNegative_pressed();

    void jog_released(); // executed when any jogging buttons above are released

    void on_xHome_clicked();
    void on_yHome_clicked();
    void on_zMax_clicked();
    void on_zUp_clicked();
    void on_zDown_clicked();
    void on_zMin_clicked();
    void on_activateHopper_stateChanged(int arg1);
    void on_connect_clicked();

    void on_activateRoller1_toggled(bool checked);
    void on_activateRoller2_toggled(bool checked);
    void on_activateJet_stateChanged(int arg1);
    void allow_user_input(bool allowed);
    void thread_ended();
    void connected_to_motion_controller();

    void print_to_output_window(QString s);
    void on_removeBuildBox_clicked();
    void on_actionShow_Hide_Console_triggered();
    void generate_printing_message_box(const std::string &message);

    void tab_was_changed(int index);

    void stop_button_pressed();
    void stop_print_and_thread();

    void get_current_x_axis_position();
    void get_current_y_axis_position();
    void get_current_z_axis_position();

    void move_z_to_absolute_position();


private:
    Ui::MainWindow *ui;
    void resizeEvent(QResizeEvent* event) override;

    Printer *mPrinter{nullptr};
    PrintThread *mPrintThread{nullptr};
    JetDrive *mJetDrive{nullptr};
    LinePrintWidget *mLinePrintingWidget{nullptr};
    QDockWidget *mDockWidget{nullptr};
    OutputWindow *mOutputWindow{nullptr};
    PowderSetupWidget *mPowderSetupWidget{nullptr};

    // Non-core modules
    JettingWidget *mJettingWidget{nullptr};
    HighSpeedLineWidget *mHighSpeedLineWidget{nullptr};
    DropletObservationWidget *mDropletObservationWidget{nullptr};

};
#endif // MAINWINDOW_H
