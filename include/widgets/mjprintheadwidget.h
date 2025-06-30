#ifndef MJPRINTHEADWIDGET_H
#define MJPRINTHEADWIDGET_H

#include <QWidget>
#include "printerwidget.h"

// Includes for STL slicing 06/24
#include <QProcess>
#include <map>


namespace Ui {
class MJPrintheadWidget;
}

struct PrintParameters {
    QString fileName = "";
    double printFrequency = 0.0;
    double printSpeed = 0.0;
    double dropletSpacingX = 0.0;
    double lineSpacingY = 0.0;
    double layerHeight = 0.0;
    double startX = 0.0;
    double startY = 0.0;
    int nozzleCount = 128;
    bool yShiftEnabled = false;
};

class MJPrintheadWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit MJPrintheadWidget(Printer *printer, QWidget *parent = nullptr);
    ~MJPrintheadWidget();
    void allow_widget_input(bool allowed) override;


protected:
    void connect_to_printhead();
    void clear_response_text();
    void command_entered();
    void powerTogglePressed();
    void getPositionPressed();
    void getHeadTempsPressed();
    void file_name_entered();
    void read_in_file(const QString &filename);
    //void send_image_data(const QString &file);

    void send_command(const QString &command);
    void frequencyChanged();
    void voltageChanged();
    void absoluteStartChanged();
    void write_to_response_window(const QString &text);
    void moveNozzleOffPlate();

    void stopPrintingPressed();
    void testPrintPressed();
    void testJetPressed();
    void createBitmapPressed();
    void singleNozzlePressed();

    void createTestBitmapsPressed();
    void variableTestPrintPressed();
    void printBMPatLocation(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileLocation);
    void printBMPatLocationEncoder(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileName);
    void moveToLocation(double xLocation, double yLocation, QString endMessage);
    void print(double acceleration, double speed, double endTargetMM, QString endMessage);
    void printEnc(double acceleration, double speed, double endTargetMM, QString endMessage);
    void verifyPrintStartAlignment(double xStart, double yStart);
    void zeroEncoder();

    void purgeNozzles();
    void testNozzles();
    QString verifyPrintStartStop(int xStart, int xStop);

    // Motion Addition:
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

    // Input allowance
    void allow_widget_input_MJ(bool allowed);

public slots:
    void on_startFullPrintButton_clicked();

private slots:
    void onStartStopDisplayClicked();
    void requestEncoderPosition();

    // STL Slicing Slots
    void sliceStlButton_clicked();
    void readPythonOutput();
    void handlePythonError();
    void onPythonScriptFinished(int exitCode, QProcess::ExitStatus exitStatus);


private:
    // Helper methods for full print job
    bool parsePrintParameters(const QString& filePath, PrintParameters& params);
    bool parseLayerShifts(const QString& filePath, std::map<int, int>& shifts);
    void startFullPrintJob(const QString& jobFolderPath);

    Ui::MJPrintheadWidget *ui;
    bool encFlag;
    QTimer *m_positionTimer;
    QStringList m_encoderHistory;
    QProcess *m_pythonProcess;          // 06/24 TODO
};

#endif // MJPRINTHEADWIDGET_H
