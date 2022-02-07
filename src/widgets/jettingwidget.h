#ifndef JETTINGWIDGET_H
#define JETTINGWIDGET_H

#include <QWidget>

#include "printerwidget.h"

namespace Ui {
class JettingWidget;
}

class JettingWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit JettingWidget(QWidget *parent = nullptr);
    ~JettingWidget();
    void allow_widget_input(bool allowed) override;

signals:
    void start_jetting();
    void stop_jetting();

private slots:
    void on_startJetting_clicked();
    void on_stopJetting_clicked();

private:
    Ui::JettingWidget *ui;

};

#endif // JETTINGWIDGET_H
