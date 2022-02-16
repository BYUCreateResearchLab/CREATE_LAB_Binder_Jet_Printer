#ifndef JETTINGWIDGET_H
#define JETTINGWIDGET_H

#include <QWidget>

#include "printerwidget.h"
#include "jetdrive.h"

namespace Ui {
class JettingWidget;
}

class JettingWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit JettingWidget(JetDrive *jetDrv, QWidget *parent = nullptr);
    ~JettingWidget();
    void allow_widget_input(bool allowed) override;

signals:
    void start_jetting();
    void stop_jetting();

private slots:
    void update_settings_clicked();

private:
    Ui::JettingWidget *ui;
    JetDrive *mJetDrive{nullptr};

};

#endif // JETTINGWIDGET_H
