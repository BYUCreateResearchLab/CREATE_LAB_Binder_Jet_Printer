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
    explicit LinePrintWidget(QWidget *parent = nullptr);
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


    // put these somewhere better soon!
    double Printer2NozzleOffsetX{-12.7};
    double Printer2NozzleOffsetY{-182.75};


    std::vector<logType> activeLogTypes = {logType::Error, logType::Status, logType::Standard, logType::Debug};
    // Value types for data input columns for printing lines
    std::vector<std::string> LinePrintDataColumnTypes = {"int", "float", "float", "int", "int", "float", "float"};
    LinePrintData table = LinePrintData();

    QPen linePen = QPen(Qt::blue, 0.1, Qt::SolidLine, Qt::RoundCap);
    QPen lineTravelPen = QPen(Qt::red, 0.1, Qt::DashLine, Qt::RoundCap);

    QString dmcLinePrintCode;

    /*const char * dmcLinePrintCode =
            R"(#BEGIN;yCnt=800;xCnt=1000;DMData[11];JS#fill("Data",0)
               #DATA_WT
               #LOOP;WT100;begin=Data[0];JP#LOOP,begin=0;JP#STOP,begin=2;JS#PRINT;Data[0]=0
               JP#DATA_WT
               #STOP;SPX=60*xCnt;SPY=40*yCnt;PAX=150*xCnt;PAY=0;BGXY;AM;EN
               #PRINT;strtX=Data[1];strtY=Data[2];numLs=Data[3];lSpce=Data[4];lDist=Data[5]
               dSpce=Data[6];jetHz=Data[7];pVelc=Data[8];pAccl=Data[9];index=Data[10]
               ACX =pAccl;DCX =pAccl;ACY =400*yCnt;DCY =400*yCnt;SHH;ACH =20000000
               accT=pVelc/pAccl;accD =accT*accT*pAccl*0.5;gearR=1000.0/(dSpce*xCnt)
               jOffD =accD +lDist
               #PRNTL;SPX=60*xCnt;SPY=80*yCnt;PAX=(strtX-accD);PAY=strtY;BGXY;AM;SPX=pVelc
               GAH=X;PRX=lDist+(2*accD);BGX;ADX=accD;GRH=gearR;ADX=jOffD;GRH=0;AM
               strtY=strtY+lSpce;index=index+1;JP#PRNTL,index<numLs;EN
               #fill;^c=0
               #fill_h;^a[^c]=^b;^c=^c+1;JP#fill_h,(^c<^a[-1]);EN)";*/

};

#endif // LINEPRINTWIDGET_H
