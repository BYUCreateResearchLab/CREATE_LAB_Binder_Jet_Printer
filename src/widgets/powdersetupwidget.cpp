#include "powdersetupwidget.h"
#include "ui_powdersetupwidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include "printer.h"
#include "dmc4080.h"
#include "mister.h"
#include "mjdriver.h"


#include <QMessageBox>
#include <QDebug>

PowderSetupWidget::PowderSetupWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::PowderSetupWidget)
{
    ui->setupUi(this);
    connect(ui->levelRecoat, &QPushButton::clicked, this, &PowderSetupWidget::level_recoat_clicked);
    connect(ui->normalRecoat, &QPushButton::clicked, this, &PowderSetupWidget::normal_recoat_clicked);

    connect(ui->mistLayerButton, &QAbstractButton::clicked, this, &PowderSetupWidget::mist_layer);
    connect(ui->toggleMisterButton, &QAbstractButton::clicked, this, &PowderSetupWidget::toggle_mister_clicked);

    connect(ui->cureLayerButton, &QPushButton::clicked, this, &PowderSetupWidget::cure_layer_pressed);

    connect(ui->start_powder_deposition, &QPushButton::clicked, this, &PowderSetupWidget::sift_powder_clicked);


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

    s << CMD::move_xy_axes_to_default_position();

    emit execute_command(s);
    emit generate_printing_message_box("Level recoat is in progress.");
}

void PowderSetupWidget::normal_recoat_clicked()
{
    double zScale = 1.0 + (ui->Z_ScaleFactor->value()) / 100.0;

    std::stringstream s;
    int numLayers{ui->recoatCyclesSpinBox->value()};
    RecoatSettings layerRecoatSettings {};
    layerRecoatSettings.isLevelRecoat = false;
    layerRecoatSettings.rollerTraverseSpeed_mm_s = ui->rollerTraverseSpeedSpinBox->value();
    layerRecoatSettings.recoatSpeed_mm_s = ui->recoatSpeedSpinBox->value();
    // Note: Index from the combo box must match up with the data to be sent over RS-232 to the generator (see documentation of generator)
    layerRecoatSettings.ultrasonicIntensityLevel = ui->ultrasonicIntensityComboBox->currentIndex();
    layerRecoatSettings.ultrasonicMode = ui->ultrasonicModeComboBox->currentIndex();
    layerRecoatSettings.layerHeight_microns = (ui->layerHeightSpinBox->value()) * zScale;
    layerRecoatSettings.waitAfterHopperOn_millisecs = ui->hopperDwellTimeMsSpinBox->value();

    s << CMD::display_message("starting normal recoat...");
    s << CMD::display_message("Z Scale Factor: ");
    s << CMD::display_message(std::to_string(zScale));
    for(int i{0}; i < numLayers; ++i)
    {
        std::string message{"spreading layer "};
        message += std::to_string(i+1);
        message += " of ";
        message += std::to_string(numLayers);
        message += "...";
        s << CMD::display_message(message);
        s << CMD::spread_layer(layerRecoatSettings);
    }
    s << CMD::display_message("powder spreading complete");
    s << CMD::display_message("");

    s << CMD::move_xy_axes_to_default_position();

    emit execute_command(s);
    emit generate_printing_message_box("Normal recoat is in progress.");
}

void PowderSetupWidget::sift_powder_clicked()
{
    std::stringstream s;
    const int sift_duration_min = ui->sift_time_min->value();
    const int sift_duration_ms = sift_duration_min * (1000 * 60);

    // Get ultrasonic settings from the UI
    int ultrasonicIntensity = ui->ultrasonicIntensityComboBox->currentIndex();
    int ultrasonicMode = ui->ultrasonicModeComboBox->currentIndex();

    // Call the command helper function to build the command string
    s << CMD::sift_powder(ultrasonicMode, ultrasonicIntensity, sift_duration_ms);

    // Emit the command to be executed by the printer controller
    emit execute_command(s);

    // Show a message box in the main application UI to inform the user
    std::string message = "Sifting powder for " + std::to_string(sift_duration_min) + " minutes.";
    emit generate_printing_message_box(message);
}

void PowderSetupWidget::allow_widget_input(bool allowed)
{
    ui->recoaterSettingsFrame->setEnabled(allowed);
    ui->mistingFrame->setEnabled(allowed);
    ui->heatLampFrame->setEnabled(allowed);
}

void PowderSetupWidget::mist_layer()
{
    std::stringstream s;
    s << "GCmd,#BEGIN\n";
    const double mistSpeed = ui->mistTraverseSpeedSpinBox->value();
    const double mistDwellTime = int(1000.0 * ui->misterDwellTimeSpinBox->value());
    s << CMD::mist_layer(mistSpeed, mistDwellTime);

    auto programString = CMD::cmd_buf_to_dmc(s);
    programString += "EN;"; // Controller doesn't like an empty line at the end
    const char* program = programString.c_str();

    if (mPrinter->mcu->g)
    {
        // upload program with up to full compression enabled on the preprocessor
        if (GProgramDownload(mPrinter->mcu->g, program, "--max 4") == G_NO_ERROR)
            qDebug() << "Program Downloaded with compression level 4";
        else
        {
            qDebug() << "Unexpected GProgramDownload() behaviour";
            return;
        }
    }

    s.clear();

    s << "GCmd," << "XQ #BEGIN" << "\n";
    s << "GProgramComplete," << "\n";
    s << CMD::display_message("Print Complete");

    emit disable_user_input();
    emit execute_command(s);
    emit generate_printing_message_box("Layer misting is in progress.");
}

void PowderSetupWidget::toggle_mister_clicked()
{
    // TODO: state machine?
    if (isMisting)
    {
        mPrinter->mister->turn_off_misters();
        ui->toggleMisterButton->setText("Turn On Mister");
        isMisting = false;
    }
    else
    {
        mPrinter->mister->turn_on_misters();
        ui->toggleMisterButton->setText("Turn Off Mister");
        isMisting = true;
    }
}

void PowderSetupWidget::cure_layer_pressed()
{
    std::stringstream s;
    Axis y {Axis::Y};
    double zAxisOffsetUnderRoller {0.5};
    const double defaultTraverseSpeed = 60.0;
    using namespace CMD;
    const double heatLampStart_mm = -310;
    const double heatLampEnd_mm = -460;

    const double heatingTraverseSpeed = ui->heatLampSpeedSpinBox->value();

    s << display_message("curing layer...");

    // move z-axis down when going back to get more powder
    s << set_accleration(Axis::Z, 10)
      << set_deceleration(Axis::Z, 10)
      << set_speed(Axis::Z, 2)
      << position_relative(Axis::Z, -zAxisOffsetUnderRoller)
      << begin_motion(Axis::Z)
      << motion_complete(Axis::Z);

    // move to just before the heat lamp
    s << set_accleration(y, 400);
    s << set_deceleration(y, 400);
    s << set_speed(y, defaultTraverseSpeed);
    s << position_absolute(y, heatLampStart_mm)
      << begin_motion(y)
      << motion_complete(y);

    // turn on heat lamp
    s << set_bit(HEAT_LAMP_BIT);

    // traverse under heat lamp
    s << set_speed(y, heatingTraverseSpeed);
    s << position_absolute(y, heatLampEnd_mm);
    s << begin_motion(y);
    s << motion_complete(y);

    // turn off heat lamp
    s << clear_bit(HEAT_LAMP_BIT);

    // move z-axis back up
    s << position_relative(Axis::Z, zAxisOffsetUnderRoller);
    // move to back of y-axis
    s << position_absolute(y, -Y_STAGE_LEN_MM);
    s << set_speed(y, defaultTraverseSpeed);
    s << begin_motion(Axis::Z);
    s << begin_motion(y);
    s << motion_complete(y);
    s << motion_complete(Axis::Z);

    s << display_message("layer cured");
    s << display_message("");


    emit execute_command(s);
    emit generate_printing_message_box("Layer cure is in progress.");

}

#include "moc_powdersetupwidget.cpp"
