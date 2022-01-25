#include "powdersetupwidget.h"
#include "ui_powdersetupwidget.h"

PowderSetupWidget::PowderSetupWidget(QWidget *parent) : QWidget(parent), ui(new Ui::PowderSetupWidget)
{
    ui->setupUi(this);
}

PowderSetupWidget::~PowderSetupWidget()
{
    delete ui;
}
