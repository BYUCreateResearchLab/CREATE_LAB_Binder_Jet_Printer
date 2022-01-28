#include "powdersetupwidget.h"
#include "ui_powdersetupwidget.h"

#include <sstream>

#include "printer.h"

#include <QMessageBox>

PowderSetupWidget::PowderSetupWidget(QWidget *parent) : QWidget(parent), ui(new Ui::PowderSetupWidget)
{
    ui->setupUi(this);
    connect(ui->levelRecoat, &QPushButton::clicked, this, &PowderSetupWidget::level_recoat_clicked);
    connect(ui->normalRecoat, &QPushButton::clicked, this, &PowderSetupWidget::normal_recoat_clicked);

    // Set combo box defaults
    ui->ultrasonicIntensityComboBox->setCurrentIndex(2); // 80%
    ui->ultrasonicModeComboBox->setCurrentIndex(3);      // Mode D
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

    for(int i{0}; i < numLayers; ++i)
    {
        s << CMD::spread_layer(levelRecoat);
    }

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

    for(int i{0}; i < numLayers; ++i)
    {
        s << CMD::spread_layer(layerRecoat);
    }

    emit execute_command(s);
    emit generate_printing_message_box("Normal recoat is in progress.");
}

void PowderSetupWidget::allow_user_input(bool allowed)
{
    ui->recoaterSettingsFrame->setEnabled(allowed);
}
