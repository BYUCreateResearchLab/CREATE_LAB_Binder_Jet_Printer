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
    // stringstreams are not copyable... is there a better way to handle this?
    // maybe I can return a reference to an interal vector of stringstreams?
    std::vector<std::string> print_commands_for_lines();

    int numLines{};
    int lineSpacing_um{};
    double lineLength_mm{};
    int dropletSpacing_um{};
    int jettingFrequency_Hz{};
    int acceleration_mm_per_s{};

    SmallBuildBox buildBox{};

    Axis printAxis{};
    Axis viewAxis{};

    int triggerOffset_ms{};

private:
    std::stringstream s_;
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
    void setup();
    void update_print_settings();
    void update_print_axes(int index);

private:
    Ui::HighSpeedLineWidget *ui;
    HighSpeedLineCommandGenerator *print{nullptr};
};

#endif // HIGHSPEEDLINEWIDGET_H
