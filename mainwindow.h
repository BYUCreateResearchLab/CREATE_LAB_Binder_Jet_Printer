#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"
#include <progwindow.h>
#include <outputwindow.h>
#include "printer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QMainWindow *parent = nullptr);
    ~MainWindow();
    void setup(Printer *printerPtr);

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
    void on_activateJet_stateChanged(int arg1);

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
    Printer *printer{nullptr};
    progWindow *sWindow;
    QDockWidget* mDockWidget;
    OutputWindow* mOutputWindow;
    void e(GReturn rc);
};
#endif // MAINWINDOW_H
