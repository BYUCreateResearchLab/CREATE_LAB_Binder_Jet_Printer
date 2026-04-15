#include "heatlampwidget.h"
#include "outputwindow.h"
#include "ui_heatlampwidget.h"
#include "heatlamp.h"
#include "mjprintheadwidget.h"
#include "mainwindow.h"
#include <QtCharts>

HeatLampWidget::HeatLampWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::HeatLampWidget)
{
    ui->setupUi(this);
    setAccessibleName("Heat Lamp Widget");

    connect(ui->getBedTempButton, &QPushButton::clicked, this, &HeatLampWidget::get_bed_temp);
    connect(ui->openConnectionToControllerButton, &QPushButton::clicked, this, &HeatLampWidget::open_connection);
    connect(ui->cureLayerButton, &QPushButton::clicked, this, &HeatLampWidget::cure_layer_pressed);
    connect(ui->clearHistoryButton, &QPushButton::clicked, this, &HeatLampWidget::clear_temperature_history);
    connect(ui->setBitsButton, &QPushButton::clicked, this, &HeatLampWidget::set_bits);
    connect(ui->setIntensityButton, &QPushButton::clicked, this, &HeatLampWidget::set_intensity);
    connect(ui->showChartButton, &QPushButton::clicked, this, &HeatLampWidget::show_chart);
    connect(ui->addFakeTempButton, &QPushButton::clicked, this, &HeatLampWidget::add_fake_temp);
    connect(ui->rollAndCureButton, &QPushButton::clicked, this, &HeatLampWidget::roll_and_cure_layers);
    connect(ui->printTemperatureDataButton, &QPushButton::clicked, this, &HeatLampWidget::print_temp_history);
}

HeatLampWidget::~HeatLampWidget()
{
    delete ui;
}

// enable/disable widgets when they should not be able to be pressed
void HeatLampWidget::allow_widget_input(bool allowed)
{
}

void HeatLampWidget::clear_temperature_history() {
    if(mPrinter -> heatLamp) {
        mPrinter -> heatLamp -> clear_history();
    }
}

void HeatLampWidget::set_intensity() {
    std::stringstream ss;
    ss << mPrinter -> heatLamp -> set_intensity(ui -> intensityInput -> value());
    mPrinter -> mcu -> printerThread -> execute_command(ss);
}

void HeatLampWidget::set_bits() {
    qDebug("set_bits pressed");
    std::stringstream ss;
    if(ui -> D0Checkbox -> isChecked()) {
        qDebug("D0");
        ss << CMD::set_bit(HEATLAMP_D0);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D0);
    }
    if(ui -> D1Checkbox -> isChecked()) {
        qDebug("D1");
        ss << CMD::set_bit(HEATLAMP_D1);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D1);
    }
    if(ui -> D2Checkbox -> isChecked()) {
        qDebug("D2");
        ss << CMD::set_bit(HEATLAMP_D2);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D2);
    }
    if(ui -> D3Checkbox -> isChecked()) {
        qDebug("D3");
        ss << CMD::set_bit(HEATLAMP_D3);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D3);
    }

    mPrinter -> mcu -> printerThread -> execute_command(ss);
}

void HeatLampWidget::open_connection() {
    std::stringstream ss;
    ss << CMD::open_connection_to_controller();
    ss << CMD::detail::GCmd("CO 3");        // configures bank 2 and 3 as outputs on extended I/O (IO 17-32)
    ss << CMD::detail::GCmd("DM BEDTEMP[1]");
    ss << CMD::detail::GCmd("DM BEDTEMPS[1000]");

    for(int i = 0; i < 1000; i++){ //clear BEDTEMPS array
        ss << CMD::detail::GCmd("BEDTEMPS[" + std::to_string(i) + "] = 0");
    }

    ss << CMD::set_bit(HEATLAMP_D0)
       << CMD::set_bit(HEATLAMP_D1)
       << CMD::set_bit(HEATLAMP_D2)
       << CMD::set_bit(HEATLAMP_D3);
    mPrinter -> mcu -> printerThread -> execute_command(ss);
}

void HeatLampWidget::get_bed_temp() {
    char buff[G_SMALL_BUFFER];
    double temp;
    ui -> text_output -> setText(QString("bed temp requested!"));
    std::stringstream ss;
    ss << CMD::deallocate_array("BEDTEMP");
    ss << CMD::define_array("BEDTEMP", 1);
    ss << CMD::detail::GCmd() + "BEDTEMP[0] = @AN[1] \n";
    mPrinter -> mcu -> printerThread -> execute_command(ss);
    for(int i = 0; i < 10000000; i++){}
    if(mPrinter->mcu->g) {
        GArrayUpload(mPrinter->mcu->g, "BEDTEMP", 0, 0, G_COMMA, buff, G_SMALL_BUFFER);
        temp = std::stod(buff);
        ui -> text_output -> setText(QString::number(temp));
    } else {
        qDebug("controller not connected");
    }
}

void HeatLampWidget::cure_layer_pressed() {
    PrintParameters settings;
    settings.cureTime_s = ui -> cureTimeInput -> value();
    settings.target_temp = ui -> targetTempInput -> value();
    settings.waitAfterHeatLampOn_millisecs = ui -> waitAfterHeatLampInput -> value();
    settings.kp = ui -> kpInput -> value();
    settings.ki = ui -> kiInput -> value();
    settings.default_intensity = ui -> defaultIntensityInput -> value();
    settings.starting_intensity = ui -> startingIntensityInput -> value();

    std::stringstream s;
    s << mPrinter -> cure_layer(settings);
    mPrinter -> mcu -> printerThread -> execute_command(s);
}

void HeatLampWidget::cure_and_roll(PrintParameters params, RecoatSettings recoatSettings) {
    // 1. Move nozzle to park position FIRST.
    //mPrinter->mjController->outputMessage("Moving nozzle to park position for recoat.");

    //MJPrintheadWidget::moveNozzleOffPlate();

    //2. cure layer
    //mPrinter->mjController->outputMessage("Performing curing operation...");
    curingComplete = false;
    std::stringstream s;
    s << CMD::display_message("Curing layer...");
    s << mPrinter -> cure_layer(params);
    s << CMD::display_message("Curing Complete");

    emit execute_command(s);

    while (!curingComplete) {
        QCoreApplication::processEvents();
    }

    // 3. Now that the head is parked, perform the recoat operation.
    //mPrinter->mjController->outputMessage("Performing recoat operation...");
    recoatComplete = false;
    s = std::stringstream();

    // --- 2. **Build and execute the recoat command** ---
    s << CMD::display_message("Recoating for new layer...");
    s << CMD::spread_layer(recoatSettings);
    s << CMD::display_message("Recoat Complete");

    emit execute_command(s);

    while (!recoatComplete) {
        QCoreApplication::processEvents();
    }
}

void HeatLampWidget::roll_and_cure_layers() {
    PrintParameters settings {};
    settings.cureTime_s = ui -> cureTimeInput -> value();
    settings.target_temp = ui -> targetTempInput -> value();
    settings.waitAfterHeatLampOn_millisecs = ui -> waitAfterHeatLampInput -> value();
    settings.kp = ui -> kpInput -> value();
    settings.ki = ui -> kiInput -> value();
    settings.kd = ui -> kdInput -> value();
    settings.default_intensity = ui -> defaultIntensityInput -> value();
    settings.starting_intensity = ui -> startingIntensityInput -> value();
    RecoatSettings recoatSettings{};
    recoatSettings.layerHeight_microns = ui->layerHeightSpinBox->value();
    recoatSettings.isLevelRecoat = false; // Normal recoat for full prints
    recoatSettings.rollerTraverseSpeed_mm_s = ui->rollerTraverseSpeedSpinBox->value();
    recoatSettings.recoatSpeed_mm_s = ui->recoatSpeedSpinBox->value();
    recoatSettings.ultrasonicIntensityLevel = ui->ultrasonicIntensityComboBox->currentIndex();
    recoatSettings.ultrasonicMode = ui->ultrasonicModeComboBox->currentIndex();
    recoatSettings.waitAfterHopperOn_millisecs = ui->hopperDwellTimeMsSpinBox->value();

    for(int i = 0; i < ui -> numLayersToRoll -> value(); i++) {
        cure_and_roll(settings, recoatSettings);
        std::vector<double> bedTempList = mPrinter -> get_last_bed_temp_list();
        double averageTemp{0};

        if (bedTempList.size() != 0) {
            double sum = std::accumulate(bedTempList.begin(), bedTempList.end(), 0.0);
            averageTemp = (sum/bedTempList.size());
        }
        temp_history.push_back(averageTemp);
        show_chart();
        std::time_t currentTime = std::time(nullptr);
        while (std::time(nullptr) < currentTime + ui -> printDelayInput -> value()) {
            QCoreApplication::processEvents();
        }
    }
}

void clearLayout(QLayout *layout) {
    if (layout == NULL)
        return;
    QLayoutItem *item;
    while((item = layout->takeAt(0))) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void HeatLampWidget::print_temp_history() {
    qDebug("Temperature (deg C)\tIntensity(0-15)");
    qDebug("------------------");
    qDebug() << mPrinter -> heatLamp -> get_temp_history(temp_history.back()).c_str();
}

void HeatLampWidget::show_chart() {
    clearLayout(ui ->chartLayout);
    QLineSeries *series = new QLineSeries();
    for(int i = 0; i < temp_history.size(); i++) {
        series->append(i + 1, temp_history.at(i));
    }
    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle("Temperature vs Layer Number");
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    ui -> chartLayout -> addWidget(chartView);
}

void HeatLampWidget::add_fake_temp() {
    temp_history.push_back(ui -> fakeTemp -> value());
    show_chart();
}

#include "moc_heatlampwidget.cpp"
