#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
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

private:
    Ui::MainWindow *ui;
    int z_position = 100;
    int delta_z;
};
#endif // MAINWINDOW_H
