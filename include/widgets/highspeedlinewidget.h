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
    std::string generate_commands_for_printing_line(int lineNum);
    std::string generate_dmc_commands_for_printing_line(int lineNum);
    std::string generate_dmc_commands_for_viewing_flat(int lineNum);

    int numLines{};
    int lineSpacing_um{};
    double lineLength_mm{};
    int dropletSpacing_um{};
    int jettingFrequency_Hz{};
    int acceleration_mm_per_s2{};

    //const double print_travel_length{20.0};
    const double xTravelSpeed{50};

    SmallBuildBox buildBox{};

    Axis printAxis{};
    Axis viewAxis{};

    int triggerOffset_ms{};

private:
    std::stringstream s_;
    int cntsPerSec{2048};
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

public slots:
    void print_line();
    void view_flat();
    void when_line_print_completed();

private slots:
    void stop_printing();
    void setup();
    void update_print_settings();
    void update_print_axes(int index);
    void allow_user_to_change_parameters(bool allowed);
    void set_x_center();
    void set_y_center();

    void move_to_build_box_center();
    void reset_print();

    void increment_line_number();
    void decrement_line_number();

private:
    Ui::HighSpeedLineWidget *ui;
    HighSpeedLineCommandGenerator *print{nullptr};
    int currentLineToPrintIndex{0};
    bool printIsRunning_{false};
};

#endif // HIGHSPEEDLINEWIDGET_H
