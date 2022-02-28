#ifndef HIGHSPEEDLINEWIDGET_H
#define HIGHSPEEDLINEWIDGET_H

#include <QWidget>
#include <QPen>

#include "printerwidget.h"
#include "printer.h"
#include <sstream>

struct SmallBuildBox
{
    double centerX{};
    double centerY{};
    double thickness{};
    double length{};
};

class HighSpeedLineCommandGenerator
{
public:
    // each string in the vector will be the code for printing a line
    std::vector<std::string> print_commands_for_lines();
    std::string generate_commands_for_printing_line(int lineNum);

    int numLines{};
    int lineSpacing_um{};
    double lineLength_mm{};
    int dropletSpacing_um{};
    int jettingFrequency_Hz{};
    int acceleration_mm_per_s2{};

    //const double print_travel_length{20.0};
    const double xTravelSpeed{150.0};

    SmallBuildBox buildBox{};

    Axis printAxis{};
    Axis viewAxis{};

    int triggerOffset_ms{};

private:
    std::stringstream s_;
    int cntsPerSec{1024};
};

namespace Ui
{
class HighSpeedLineWidget;
}

class HighSpeedLineWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit HighSpeedLineWidget(QWidget *parent = nullptr);
    ~HighSpeedLineWidget();
    void allow_widget_input(bool allowed) override;
    void reset_preview_zoom();

private slots:
    void print_line();
    void stop_printing();
    void setup();
    void update_print_settings();
    void update_print_axes(int index);

private:
    Ui::HighSpeedLineWidget *ui;
    HighSpeedLineCommandGenerator *print{nullptr};
    int currentLineToPrintIndex{0};
};

#endif // HIGHSPEEDLINEWIDGET_H
