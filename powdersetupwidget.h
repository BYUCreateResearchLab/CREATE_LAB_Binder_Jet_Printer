#ifndef POWDERSETUPWIDGET_H
#define POWDERSETUPWIDGET_H

#include <QWidget>

namespace Ui {
class PowderSetupWidget;
}

class PowderSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PowderSetupWidget(QWidget *parent = nullptr);
    ~PowderSetupWidget();

private:
    Ui::PowderSetupWidget *ui;
};

#endif // POWDERSETUPWIDGET_H
