#ifndef MJPRINTHEADWIDGET_H
#define MJPRINTHEADWIDGET_H

#include <QWidget>

#include "printerwidget.h"

namespace Ui {
class MJPrintheadWidget;
}

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
    void stopPrintingPressed();
    void testPrintPressed();
    void testJetPressed();
    void createBitmapPressed();
    void singleNozzlePressed();

    void createTestBitmapsPressed();
    void variableTestPrintPressed();
    void printBMPatLocation(double xLocation, double yLocation, double frequency, double printSpeed, int imageWidth, QString fileLocation);


private:
    Ui::MJPrintheadWidget *ui;
};

#endif // MJPRINTHEADWIDGET_H
