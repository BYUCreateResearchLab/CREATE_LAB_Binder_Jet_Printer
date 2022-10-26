#include "dropletgraphwindow.h"
#include <QWidget>
#include <QVBoxLayout>

DropletGraphWindow::DropletGraphWindow(QWidget *parent) :
    QMainWindow(parent)
{
    m_centralWidget = new QWidget(this);
    m_centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
    m_verticalLayout = new QVBoxLayout(m_centralWidget);
    m_verticalLayout->setSpacing(6);
    m_verticalLayout->setContentsMargins(11, 11, 11, 11);
    m_verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    m_plot = new QCustomPlot(this);

    m_verticalLayout->addWidget(m_plot);

    setCentralWidget(m_centralWidget);

    setGeometry(400, 250, 542, 390);
    setWindowTitle("Droplet Tracking");
}

void DropletGraphWindow::plot_data(const std::vector<double> &x, const std::vector<double> &y)
{
    // add two new graphs and set their look:
    m_plot->addGraph(); // graph 0
    m_plot->graph(0)->setPen(QPen(Qt::black)); // line color blue for first graph
    //graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue

    m_plot->axisRect()->setupFullAxesBox(true);

    // pass data points to graphs:
    m_plot->graph(0)->setData(QVector<double>(x.begin(), x.end()),
                              QVector<double>(y.begin(), y.end()));

    m_plot->xAxis->setLabel("Time (µs)");
    m_plot->yAxis->setLabel("Distance (µm)");

    // set scatter style
    m_plot->graph()->setScatterStyle(QCPScatterStyle::ssPlus);

    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    m_plot->graph(0)->rescaleAxes();

    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    m_plot->replot();
}

void DropletGraphWindow::plot_trend_line(const std::vector<double> &x, const std::vector<double> &y)
{
    m_plot->addGraph(); // graph 0
    m_plot->graph(1)->setPen(QPen(Qt::red));


    m_plot->graph(1)->setData(QVector<double>(x.begin(), x.end()),
                              QVector<double>(y.begin(), y.end()));
    m_plot->graph(1)->setScatterStyle(QCPScatterStyle::ssNone);
    m_plot->replot();
}

#include "moc_dropletgraphwindow.cpp"
