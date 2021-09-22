#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// Use this as well as commenting out the headers and cpp files in the .pro file to avoid loading gclib for UI testing
//#define AVOIDGCLIB



#ifdef AVOIDGCLIB
#include "fakegclib.h"
#endif

#ifndef AVOIDGCLIB
#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"
#endif





#include <progwindow.h>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QMainWindow *parent = nullptr);
    ~MainWindow();

private slots:
    void on_yPositive_clicked();

    void on_xPositive_clicked();

    void on_yNegative_clicked();

    void on_xNegative_clicked();

    void on_xHome_clicked();

    void on_yHome_clicked();

    void on_zStepSize_valueChanged(int arg1);

    void on_zMax_clicked();

    void on_zUp_clicked();

    void on_zDown_clicked();

    void on_zMin_clicked();

    void on_activateRoller1_stateChanged(int arg1);

    void on_activateRoller2_stateChanged(int arg1);

    void on_activateHopper_stateChanged(int arg1);

    void on_connect_clicked();

    void on_OpenProgramWindow_clicked();

    void on_saveDefault_clicked();

    void on_revertDefault_clicked();

    void on_zHome_clicked();

    void on_spreadNewLayer_clicked();

    void on_activateRoller1_toggled(bool checked);

    void on_activateRoller2_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    int z_position = 100;
    int delta_x;
    int delta_y;
    int delta_z;
    int micronX;
    int micronY;
    int micronZ;
    int mmX;
    int mmY;
    int mmZ;
    char const *address = "192.168.42.100";
    GCon g = 0; // Handle for connection to Galil Motion Controller
    progWindow *sWindow;
};
#endif // MAINWINDOW_H
