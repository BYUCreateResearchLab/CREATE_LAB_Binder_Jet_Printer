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
    explicit JettingWidget(Printer *printer, QWidget *parent = nullptr);
    ~JettingWidget();
    void allow_widget_input(bool allowed) override;
    const JetDrive::Settings& get_jet_drive_settings();

signals:
    void start_jetting();
    void stop_jetting();

private slots:
    void update_settings_clicked();
    void update_ui(const JetDrive::Settings& settings);

private:
    Ui::JettingWidget *ui;
    JetDrive::Settings m_jetSettings;

};

#endif // JETTINGWIDGET_H
