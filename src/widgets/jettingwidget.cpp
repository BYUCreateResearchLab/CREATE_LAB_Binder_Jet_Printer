#include "jettingwidget.h"
#include "ui_jettingwidget.h"

#include "QDebug"

JettingWidget::JettingWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::JettingWidget)
{
    ui->setupUi(this);
    setAccessibleName("Jetting Widget");
    connect(ui->updateSettingsButton, &QPushButton::clicked, this, &JettingWidget::update_settings_clicked);
    update_ui(mPrinter->jetDrive->get_jetting_parameters());
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

const JetDrive::Settings& JettingWidget::get_jet_drive_settings()
{
    return m_jetSettings;
}

// Note: Waveform defaults are set in jetdrive.cpp and then updated here
void JettingWidget::update_settings_clicked()
{
    // THE JETDRIVE MUST NOT BE JETTING WHEN THE WAVEFORM IS UPDATED, THIS CAN BREAK THE JETDRIVE
    // ensure that the jet drive is not jetting by setting to internal trigger, single jet and not sending any signals
    // then set back to external triger after the waveform is complete, so that if we were jetting it goes back to jetting
    if (mPrinter->jetDrive->is_connected())
    {
        mPrinter->jetDrive->set_internal_trigger();
        mPrinter->jetDrive->set_num_drops_per_trigger(1);
        mPrinter->jetDrive->set_single_jetting();
        JetDrive::Waveform waveform;
        waveform.fTRise = ui->riseTime1SpinBox->value();
        waveform.fTDwell = ui->dwellTimeSpinBox->value();
        waveform.fTFall = ui->fallTimeSpinBox->value();
        waveform.fTEcho = ui->echoTimeSpinBox->value();
        waveform.fTFinal = ui->riseTime2SpinBox->value();
        waveform.fUIdle = ui->idleVoltageSpinBox->value();
        waveform.fUDwell = ui->dwellVoltageSpinBox->value();
        waveform.fUEcho = ui->echoVoltageSpinBox->value();

        mPrinter->jetDrive->set_waveform(waveform);
        mPrinter->jetDrive->set_external_trigger(); // will start jetting again if we were jetting previously
    }
    else emit print_to_output_window("Waveform update error. JetDrive is not connected!");
}

void JettingWidget::update_ui(const JetDrive::Settings &settings)
{
    m_jetSettings = settings;
    ui->riseTime1SpinBox->setValue(settings.waveform.fTRise);
    ui->dwellTimeSpinBox->setValue(settings.waveform.fTDwell);
    ui->fallTimeSpinBox->setValue(settings.waveform.fTFall);
    ui->echoTimeSpinBox->setValue(settings.waveform.fTEcho);
    ui->riseTime2SpinBox->setValue(settings.waveform.fTFinal);
    ui->idleVoltageSpinBox->setValue(settings.waveform.fUIdle);
    ui->dwellVoltageSpinBox->setValue(settings.waveform.fUDwell);
    ui->echoVoltageSpinBox->setValue(settings.waveform.fUEcho);
}

#include "moc_jettingwidget.cpp"
