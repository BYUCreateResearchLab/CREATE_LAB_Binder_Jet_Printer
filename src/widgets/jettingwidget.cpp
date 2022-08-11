#include "jettingwidget.h"
#include "ui_jettingwidget.h"

#include "QDebug"

JettingWidget::JettingWidget(JetDrive *jetDrv, QWidget *parent) :
    PrinterWidget(parent),
    ui(new Ui::JettingWidget),
    mJetDrive(jetDrv)
{
    ui->setupUi(this);
    setAccessibleName("Jetting Widget");
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

// Note: currently the JetDrive settings set during startup are hardcoded in jetdrive.cpp. Changing the defaults in the .ui file for
//       this wdiget currently does not change the startup values, only the display defaults. work on fixing this
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
        MicroJet jetSettings;
        jetSettings.fTRise = ui->riseTime1SpinBox->value();
        jetSettings.fTDwell = ui->dwellTimeSpinBox->value();
        jetSettings.fTFall = ui->fallTimeSpinBox->value();
        jetSettings.fTEcho = ui->echoTimeSpinBox->value();
        jetSettings.fTFinal = ui->riseTime2SpinBox->value();
        jetSettings.fUIdle = ui->idleVoltageSpinBox->value();
        jetSettings.fUDwell = ui->dwellVoltageSpinBox->value();
        jetSettings.fUEcho = ui->echoVoltageSpinBox->value();
        mJetDrive->set_waveform(jetSettings);
        mJetDrive->set_external_trigger(); // will start jetting again if we were jetting previously
    }
    else qDebug() << "the JetDrive is not connected!";
}

