#ifndef HIGHSPEEDLINEWIDGET_H
#define HIGHSPEEDLINEWIDGET_H

#include <QWidget>

#include "printerwidget.h"

namespace Ui {
class HighSpeedLineWidget;
}

class HighSpeedLineWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit HighSpeedLineWidget(QWidget *parent = nullptr);
    ~HighSpeedLineWidget();
    void allow_widget_input(bool allowed) override;


private slots:
    void on_pushButton_clicked();

private:
    Ui::HighSpeedLineWidget *ui;
};

#endif // HIGHSPEEDLINEWIDGET_H
