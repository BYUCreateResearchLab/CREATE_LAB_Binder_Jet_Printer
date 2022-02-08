#ifndef DROPLETOBSERVATIONWIDGET_H
#define DROPLETOBSERVATIONWIDGET_H

#include <QWidget>
#include "printerwidget.h"
#include <QMdiArea>
#include <ueye.h>

namespace Ui {
class DropletObservationWidget;
}

class DropletObservationWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit DropletObservationWidget(QWidget *parent = nullptr);
    ~DropletObservationWidget();
    void allow_widget_input(bool allowed) override;

private slots:
    void connect_to_camera();
    void set_settings();

private:
    Ui::DropletObservationWidget *ui;
    HIDS mCamera{0};
};

#endif // DROPLETOBSERVATIONWIDGET_H
