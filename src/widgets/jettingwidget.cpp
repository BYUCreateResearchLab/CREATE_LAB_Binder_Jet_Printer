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
    update_ui(mJetDrive->get_jet_drive_settings());
    // can't call update_settings_clicked() here because it will try to send commands to the JetDrive which is not connected on startup...
}

JettingWidget::~JettingWidget()
{
    delete ui;
}

void JettingWidget::allow_widget_input(bool allowed)
{
    ui->jetSettingsFrame->setEnabled(allowed);
}

const MicroJet &JettingWidget::get_jet_drive_settings()
{
    return m_jetSettings;
}

// Note: Waveform defaults are set in jetdrive.cpp and then updated here
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
        m_jetSettings.fTRise = ui->riseTime1SpinBox->value();
        m_jetSettings.fTDwell = ui->dwellTimeSpinBox->value();
        m_jetSettings.fTFall = ui->fallTimeSpinBox->value();
        m_jetSettings.fTEcho = ui->echoTimeSpinBox->value();
        m_jetSettings.fTFinal = ui->riseTime2SpinBox->value();
        m_jetSettings.fUIdle = ui->idleVoltageSpinBox->value();
        m_jetSettings.fUDwell = ui->dwellVoltageSpinBox->value();
        m_jetSettings.fUEcho = ui->echoVoltageSpinBox->value();
        mJetDrive->set_waveform(m_jetSettings);
        mJetDrive->set_external_trigger(); // will start jetting again if we were jetting previously
    }
    else qDebug() << "the JetDrive is not connected!";
}

void JettingWidget::update_ui(const MicroJet &settings)
{
    m_jetSettings = settings;
    ui->riseTime1SpinBox->setValue(settings.fTRise);
    ui->dwellTimeSpinBox->setValue(settings.fTDwell);
    ui->fallTimeSpinBox->setValue(settings.fTFall);
    ui->echoTimeSpinBox->setValue(settings.fTEcho);
    ui->riseTime2SpinBox->setValue(settings.fTFinal);
    ui->idleVoltageSpinBox->setValue(settings.fUIdle);
    ui->dwellVoltageSpinBox->setValue(settings.fUDwell);
    ui->echoVoltageSpinBox->setValue(settings.fUEcho);
}

#include "moc_jettingwidget.cpp"
