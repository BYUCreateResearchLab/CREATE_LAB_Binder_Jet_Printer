#ifndef LINEPRINTWIDGET_H
#define LINEPRINTWIDGET_H

#include <QWidget>
#include <QtSvg>
#include <QGraphicsView>
#include <QSvgWidget>
#include <QGraphicsSvgItem>
#include <array>

#include "lineprintdata.h"
#include "printerwidget.h"

namespace Ui {
class LinePrintWidget;
}

class LinePrintWidget : public PrinterWidget
{
    Q_OBJECT

    enum logType {Error, Debug, Status, Standard};

public:
    explicit LinePrintWidget(Printer *printer, QWidget *parent = nullptr);
    ~LinePrintWidget();

    void CheckCell(int row, int column);
    //void CheckTable(int row, int column);
    void updateCell(int row, int column);
    void updateTable(bool updateVerticalHeaders, bool updateHorizontalHeaders);
    void log(QString message, enum logType messageType);
    void updatePreviewWindow();
    void checkMinMax(int r, int c, float val, float min, float max, bool isInt, bool &ok);
    void generate_line_set_commands(int setNum, std::stringstream &s);

    void allow_widget_input(bool allowed) override;

private slots:
    void on_numSets_valueChanged(int arg1);
    void on_tableWidget_cellChanged(int row, int column);
    void on_startX_valueChanged(double arg1);
    void on_startY_valueChanged(double arg1);
    void on_setSpacing_valueChanged(double arg1);
    void on_printPercentSlider_sliderMoved(int position);
    void on_clearConsole_clicked();
    void print_lines_old();
    void print_lines_dmc();
    void when_line_print_completed();
    void stop_print_button_pressed();
    QString read_dmc_code(QString filename);

private:
    Ui::LinePrintWidget *ui;

    void disable_velocity_input();
    void check_x_start();

    std::vector<std::array<int, 11>> generate_line_set_arrays_dmc();
    std::string line_set_arrays_dmc();

    bool printIsRunning_{false};


    // TODO: put these somewhere better soon!
    double Printer2NozzleOffsetX{-12.7};
    double Printer2NozzleOffsetY{-177.75};


    std::vector<logType> activeLogTypes = {logType::Error, logType::Status, logType::Standard, logType::Debug};
    // Value types for data input columns for printing lines
    std::vector<std::string> LinePrintDataColumnTypes = {"int", "float", "float", "int", "int", "float", "float"};
    LinePrintData table = LinePrintData();

    QPen linePen = QPen(QColor(42, 130, 218), 0.1, Qt::SolidLine, Qt::RoundCap);
    QPen lineTravelPen = QPen(Qt::red, 0.1, Qt::DashLine, Qt::RoundCap);

    QString dmcLinePrintCode;
    QString dmcLinePrintCode_JetFreq;

};

#endif // LINEPRINTWIDGET_H
