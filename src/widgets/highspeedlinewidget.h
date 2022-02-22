#ifndef HIGHSPEEDLINEWIDGET_H
#define HIGHSPEEDLINEWIDGET_H

#include <QWidget>
#include <QPen>

#include "printerwidget.h"

namespace Ui
{
class HighSpeedLineWidget;
}

class HighSpeedLineWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit HighSpeedLineWidget(QWidget *parent = nullptr);
    ~HighSpeedLineWidget();
    void allow_widget_input(bool allowed) override;
    void reset_preview_zoom();

private slots:
    void print_line();
    void setup();

private:
    Ui::HighSpeedLineWidget *ui;
};

#endif // HIGHSPEEDLINEWIDGET_H
