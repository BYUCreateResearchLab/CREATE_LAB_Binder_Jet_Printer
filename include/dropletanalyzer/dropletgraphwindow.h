#ifndef DROPLETGRAPHWINDOW_H
#define DROPLETGRAPHWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include <vector>

class DropletGraphWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DropletGraphWindow(QWidget *parent = nullptr);
    void plot_data(const std::vector<double>& x,
                   const std::vector<double>& y);
    void plot_trend_line(const std::vector<double>& x,
                         const std::vector<double>& y);


private:
    QWidget *m_centralWidget;
    QVBoxLayout *m_verticalLayout;
    QStatusBar *m_statusBar;
    QCustomPlot *m_plot;
};

#endif // DROPLETGRAPHWINDOW_H
