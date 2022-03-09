#include "powdersetupwidget.h"
#include "ui_powdersetupwidget.h"

#include <sstream>

#include "printer.h"

#include <QMessageBox>

PowderSetupWidget::PowderSetupWidget(QWidget *parent) : PrinterWidget(parent), ui(new Ui::PowderSetupWidget)
{
    ui->setupUi(this);
    connect(ui->levelRecoat, &QPushButton::clicked, this, &PowderSetupWidget::level_recoat_clicked);
    connect(ui->normalRecoat, &QPushButton::clicked, this, &PowderSetupWidget::normal_recoat_clicked);

    connect(ui->mistLayerButton, &QAbstractButton::clicked, this, &PowderSetupWidget::mist_layer);
    connect(ui->toggleMisterButton, &QAbstractButton::clicked, this, &PowderSetupWidget::toggle_mister_clicked);

    setAccessibleName("Powder Setup Widget");

    // Set combo box defaults
    ui->ultrasonicIntensityComboBox->setCurrentIndex(5); // 50%
    ui->ultrasonicModeComboBox->setCurrentIndex(0);      // Mode A
}

PowderSetupWidget::~PowderSetupWidget()
{
    delete ui;
}

void PowderSetupWidget::level_recoat_clicked()
{
    std::stringstream s;
    int numLayers{ui->recoatCyclesSpinBox->value()};
    RecoatSettings levelRecoat{};
    levelRecoat.isLevelRecoat = true;
    levelRecoat.rollerTraverseSpeed_mm_s = ui->rollerTraverseSpeedSpinBox->value();
    levelRecoat.recoatSpeed_mm_s = ui->recoatSpeedSpinBox->value();
    // Index from the combo box must match up with the data to be sent over RS-232 to the generator (see documentation for generator)
    levelRecoat.ultrasonicIntensityLevel = ui->ultrasonicIntensityComboBox->currentIndex();
    levelRecoat.ultrasonicMode = ui->ultrasonicModeComboBox->currentIndex();
    levelRecoat.layerHeight_microns = ui->layerHeightSpinBox->value();
    levelRecoat.waitAfterHopperOn_millisecs = ui->hopperDwellTimeMsSpinBox->value();

    s << CMD::display_message("starting level recoat...");
    for(int i{0}; i < numLayers; ++i)
    {
        std::string message{"spreading layer "};
        message += std::to_string(i+1);
        message += " of ";
        message += std::to_string(numLayers);
        message += "...";
        s << CMD::display_message(message);
        s << CMD::spread_layer(levelRecoat);
    }
    s << CMD::display_message("powder spreading complete");
    s << CMD::display_message("");

    emit execute_command(s);
    emit generate_printing_message_box("Level recoat is in progress.");
}

void PowderSetupWidget::normal_recoat_clicked()
{
    std::stringstream s;
    int numLayers{ui->recoatCyclesSpinBox->value()};
    RecoatSettings layerRecoat{};
    layerRecoat.isLevelRecoat = false;
    layerRecoat.rollerTraverseSpeed_mm_s = ui->rollerTraverseSpeedSpinBox->value();
    layerRecoat.recoatSpeed_mm_s = ui->recoatSpeedSpinBox->value();
    // Index from the combo box must match up with the data to be sent over RS-232 to the generator (see documentation for generator)
    layerRecoat.ultrasonicIntensityLevel = ui->ultrasonicIntensityComboBox->currentIndex();
    layerRecoat.ultrasonicMode = ui->ultrasonicModeComboBox->currentIndex();
    layerRecoat.layerHeight_microns = ui->layerHeightSpinBox->value();
    layerRecoat.waitAfterHopperOn_millisecs = ui->hopperDwellTimeMsSpinBox->value();

    s << CMD::display_message("starting normal recoat...");
    for(int i{0}; i < numLayers; ++i)
    {
        std::string message{"spreading layer "};
        message += std::to_string(i+1);
        message += " of ";
        message += std::to_string(numLayers);
        message += "...";
        s << CMD::display_message(message);
        s << CMD::spread_layer(layerRecoat);
    }
    s << CMD::display_message("powder spreading complete");
    s << CMD::display_message("");

    emit execute_command(s);
    emit generate_printing_message_box("Normal recoat is in progress.");
}

void PowderSetupWidget::allow_widget_input(bool allowed)
{
    ui->recoaterSettingsFrame->setEnabled(allowed);
    ui->mistingFrame->setEnabled(allowed);
}

void PowderSetupWidget::mist_layer()
{
    std::stringstream s;
    s << CMD::mist_layer(ui->mistTraverseSpeedSpinBox->value());

    if (isMisting)
    {
        isMisting = false;
        s << CMD::clear_bit(MISTER_BIT);
        ui->toggleMisterButton->setText("Turn On Mister");
    }
    emit disable_user_input();
    emit execute_command(s);
    emit generate_printing_message_box("Misting Layer");
}

void PowderSetupWidget::toggle_mister_clicked()
{
    std::stringstream s;
    if (isMisting)
    {
        ui->toggleMisterButton->setText("Turn On Mister");
        s << CMD::clear_bit(MISTER_BIT);
        isMisting = false;
    }
    else
    {
        ui->toggleMisterButton->setText("Turn Off Mister");
        s << CMD::set_bit(MISTER_BIT);
        isMisting = true;
    }
    emit execute_command(s);
}
