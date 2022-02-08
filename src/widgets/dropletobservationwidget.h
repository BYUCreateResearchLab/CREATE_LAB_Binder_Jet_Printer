#ifndef DROPLETOBSERVATIONWIDGET_H
#define DROPLETOBSERVATIONWIDGET_H

#include <QWidget>
#include "printerwidget.h"
#include <QMdiArea>

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

private:
    Ui::DropletObservationWidget *ui;
};

#endif // DROPLETOBSERVATIONWIDGET_H
