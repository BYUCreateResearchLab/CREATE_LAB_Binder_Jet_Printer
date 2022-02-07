#include "jettingwidget.h"
#include "ui_jettingwidget.h"

JettingWidget::JettingWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::JettingWidget)
{
    ui->setupUi(this);
}

JettingWidget::~JettingWidget()
{
    delete ui;
}

void JettingWidget::allow_widget_input(bool allowed)
{
    ui->startJetting->setEnabled(allowed);
    ui->stopJetting->setEnabled(allowed);
}

void JettingWidget::on_startJetting_clicked()
{
    emit start_jetting();
}

void JettingWidget::on_stopJetting_clicked()
{
    emit stop_jetting();
}

