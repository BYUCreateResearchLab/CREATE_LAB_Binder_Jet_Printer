#ifndef PROGWINDOW_H
#define PROGWINDOW_H

#include <QWidget>

// #include <QMainWindow>
#include <QtSvg>
#include <QGraphicsView>
#include <QSvgWidget>
#include <QGraphicsSvgItem>

#include "lineprintdata.h"
#include "printobject.h"

namespace Ui {
class progWindow;
}

class progWindow : public QWidget
{
    Q_OBJECT

    enum logType {Error, Debug, Status, Standard};

public:
    explicit progWindow(QWidget *parent = nullptr);
    ~progWindow();

    void CheckCell(int row, int column);
    //void CheckTable(int row, int column);
    void updateCell(int row, int column);
    void updateTable(bool updateVerticalHeaders, bool updateHorizontalHeaders);
    void log(QString message, enum logType messageType);
    void updatePreviewWindow();
    void checkMinMax(int r, int c, float val, float min, float max, bool isInt, bool &ok);


signals:
    void firstWindow();

private slots:
    void on_back2Home_clicked();

    void on_numSets_valueChanged(int arg1);

    void on_tableWidget_cellChanged(int row, int column);

    void on_startX_valueChanged(double arg1);

    void on_startY_valueChanged(double arg1);

    void on_setSpacing_valueChanged(double arg1);

    void on_printPercentSlider_sliderMoved(int position);

    void on_clearConsole_clicked();

private:
    Ui::progWindow *ui;


    std::vector<logType> activeLogTypes = {logType::Error, logType::Status, logType::Standard, logType::Debug};
    // Value types for data input columns for printing lines
    std::vector<std::string> LinePrintDataColumnTypes = {"int","float","float","int", "int", "float"};
    LinePrintData table = LinePrintData();



    QPen linePen = QPen(Qt::blue, 1.0, Qt::SolidLine, Qt::RoundCap);
    QPen lineTravelPen = QPen(Qt::red, 0.5, Qt::SolidLine, Qt::RoundCap);
};

#endif // PROGWINDOW_H
