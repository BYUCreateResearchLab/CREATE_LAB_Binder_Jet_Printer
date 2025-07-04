#ifndef POWDERSETUPWIDGET_H
#define POWDERSETUPWIDGET_H

#include <QWidget>
#include <sstream>

#include "printerwidget.h"

namespace Ui {
class PowderSetupWidget;
}

class PowderSetupWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit PowderSetupWidget(Printer *printer, QWidget *parent = nullptr);
    ~PowderSetupWidget();

    void allow_widget_input(bool allowed) override;


private slots:
    void level_recoat_clicked();
    void normal_recoat_clicked();

    void mist_layer();
    void toggle_mister_clicked();

    void cure_layer_pressed();
    void sift_powder_clicked();

private:
    Ui::PowderSetupWidget *ui;
    bool isMisting{false};
};

#endif // POWDERSETUPWIDGET_H
