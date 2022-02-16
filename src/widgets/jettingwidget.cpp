#include "jettingwidget.h"
#include "ui_jettingwidget.h"

#include "QDebug"

JettingWidget::JettingWidget(JetDrive *jetDrv, QWidget *parent) :
    PrinterWidget(parent),
    ui(new Ui::JettingWidget),
    mJetDrive(jetDrv)
{
    ui->setupUi(this);
    connect(ui->updateSettingsButton, &QPushButton::clicked, this, &JettingWidget::update_settings_clicked);
}

JettingWidget::~JettingWidget()
{
    delete ui;
}

void JettingWidget::allow_widget_input(bool allowed)
{
    ui->jetSettingsFrame->setEnabled(allowed);
}

void JettingWidget::update_settings_clicked()
{
    // THE JETDRIVE MUST NOT BE JETTING WHEN THE WAVEFORM IS UPDATED, THIS CAN BREAK THE JETDRIVE
    // ensure that the jet drive is not jetting by setting to internal trigger, single jet and not sending any signals
    // then set back to external triger after the waveform is complete, so that if we were jetting it goes back to jetting
    if (mJetDrive->is_connected())
    {
        mJetDrive->set_internal_trigger();
        mJetDrive->set_num_drops_per_trigger(1);
        mJetDrive->set_single_jetting();
        mJetDrive->set_echo_and_dwell_voltage(ui->echoVoltageSpinBox->value(), ui->dwellVoltageSpinBox->value());
        mJetDrive->set_external_trigger(); // will start jetting again if we were jetting previously
    }
    else qDebug() << "the JetDrive is not connected!";
}

