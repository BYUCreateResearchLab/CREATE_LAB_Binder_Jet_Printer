#ifndef MJPRINTHEADWIDGET_H
#define MJPRINTHEADWIDGET_H

#include <QWidget>
#include "printerwidget.h"

// Includes for STL slicing & processes
#include <QProcess>
#include <map>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class QProgressDialog;

namespace Ui {
class MJPrintheadWidget;
}

class MJPrintheadWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit MJPrintheadWidget(Printer *printer, QWidget *parent = nullptr);
    ~MJPrintheadWidget();

    // Core logic override
    void allow_widget_input(bool allowed) override;
    void cure_and_roll(PrintParameters params);

protected:
    // --- Connectivity and Main Status ---
    void connect_to_printhead();
    void clear_response_text();
    void command_entered();
    void powerTogglePressed();
    void getPositionPressed();
    void getHeadTempsPressed();
    void file_name_entered();
    void read_in_file(const QString &filename, int headIdx = 1);
    void send_command(const QString &command);
    void write_to_response_window(const QString &text);

    // --- Settings SpinBoxes ---
    void frequencyChanged();
    void voltageChanged();
    void absoluteStartChanged();

    // --- Maintenance (The Survivors) ---
    void purgeNozzles();
    void moveNozzleOffPlate();

    // --- Core Movement & Printing Logic ---
    void moveToLocation(double xLocation, double yLocation, QString endMessage);
    void print(double acceleration, double speed, double endTargetMM, QString endMessage);
    void printEnc(double acceleration, double speed, double endTargetMM, QString endMessage);
    void printBMPatLocationEncoder(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName);

    // --- Motion Controls (X/Y/Z) ---
    void x_right_button_pressed_MJ();
    void x_left_button_pressed_MJ();
    void y_up_button_pressed_MJ();
    void y_down_button_pressed_MJ();
    void jog_released_MJ();
    void on_xHome_clicked_MJ();
    void on_yHome_clicked_MJ();
    void on_zUp_clicked_MJ();
    void on_zDown_clicked_MJ();
    void on_zMax_clicked_MJ();
    void on_zMin_clicked_MJ();
    void get_current_x_axis_position_MJ();
    void get_current_y_axis_position_MJ();
    void get_current_z_axis_position_MJ();
    void move_z_to_absolute_position_MJ();

    // --- Recoater Logic ---
    void levelRecoat_MJ();
    void normalRecoat_MJ();
    void performRecoat(const PrintParameters* params, bool usePrintParameters);
    void reRollLayer();

    // --- Input allowance ---
    void allow_widget_input_MJ(bool allowed);

public slots:
    void on_startFullPrintButton_clicked();
    void cancelPrintJob();

private slots:
    // --- Head Management (Transplanted from image_9dcf1f.png) ---
    void on_fillHeadButton_clicked();        // New "Fill Head" button
    void on_headSelector_currentIndexChanged(int index); // "Head Index" dropdown
    void on_clearHeadButton_clicked();       // "Clear Head" button
    void on_fillGapButton_clicked();
    void on_fillNozzleButton_clicked();
    void on_comboMode_currentIndexChanged(int index);
    void updateStatusTable(const json &j);

    // --- Slicing & Printing Slots ---
    void sliceStlButton_clicked();
    void checkMapsPressed();
    void stopPrintingPressed();
    void readPythonOutput();
    void handlePythonError();
    void onPythonScriptFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRollerButtonClicked();
    void requestEncoderPosition();

private:
    // Helper methods for full print jobs
    bool parsePrintParameters(const QString& filePath, PrintParameters& params);
    bool parseLayerShifts(const QString& filePath, std::map<int, int>& shifts);
    void startFullPrintJob(const QString& jobFolderPath);
    int calculate_gap(const QString& associatedBitmap);
    bool readyHeads();
    int m_selectedHead = 1;

    // Internal state
    volatile bool m_printJobCancelled = false;
    QProgressDialog* m_printStatusDialog = nullptr;
    Ui::MJPrintheadWidget *ui;

    QTimer *m_positionTimer;
    QProcess *m_pythonProcess;

    bool m_isRollerOn;
};

#endif // MJPRINTHEADWIDGET_H
