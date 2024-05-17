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
    void send_image_data(const QString &file);

    void send_command(const QString &command);

    void frequencyChanged();

    void voltageChanged();

    void absoluteStartChanged();

    void write_to_response_window(const QString &text);

private:
    Ui::MJPrintheadWidget *ui;
};

#endif // MJPRINTHEADWIDGET_H
