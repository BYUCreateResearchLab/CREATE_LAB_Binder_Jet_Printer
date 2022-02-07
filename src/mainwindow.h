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
    // change these over to be jogs that go while the button is pushed and stop when it is released
    void on_yPositive_pressed();
    void on_xPositive_pressed();
    void on_yNegative_pressed();
    void on_xNegative_pressed();

    void jog_released();

    void on_xHome_clicked();
    void on_yHome_clicked();
    void on_zMax_clicked();
    void on_zUp_clicked();
    void on_zDown_clicked();
    void on_zMin_clicked();
    void on_activateHopper_stateChanged(int arg1);
    void on_connect_clicked();
    void on_saveDefault_clicked();
    void on_revertDefault_clicked();
    void on_activateRoller1_toggled(bool checked);
    void on_activateRoller2_toggled(bool checked);
    void on_activateJet_stateChanged(int arg1);
    void allow_user_input(bool allowed);
    void thread_ended();
    void connected_to_motion_controller();

    void on_removeBuildBox_clicked();
    void on_actionShow_Hide_Console_triggered();
    void generate_printing_message_box(const std::string &message);

    void start_jetting();
    void stop_jetting();
private:
    Ui::MainWindow *ui;

    Printer *mPrinter{nullptr};
    PrintThread *mPrintThread{nullptr};
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
