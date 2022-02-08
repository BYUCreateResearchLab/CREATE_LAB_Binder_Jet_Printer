#ifndef DROPLETOBSERVATIONWIDGET_H
#define DROPLETOBSERVATIONWIDGET_H

#include <QWidget>
#include "printerwidget.h"
#include <QMdiArea>

class HIDS;

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
    HIDS *mCamera{nullptr};
};

#endif // DROPLETOBSERVATIONWIDGET_H
