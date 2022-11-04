#ifndef DROPLETANALYZERWIDGET_H
#define DROPLETANALYZERWIDGET_H

#include <QWidget>
#include <QMainWindow>

#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "jetdrive.h"

class DropletAnalyzer;
class ImageViewer;
class QHBoxLayout;
class QVBoxLayout;
class QThread;
class DropletGraphWindow;
class QCustomPlot;

//QT_BEGIN_NAMESPACE
//namespace KDToolBox {class KDGenericSignalThrottler; }
//QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui { class DropletAnalyzerWidget; }
QT_END_NAMESPACE

class DropletAnalyzerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DropletAnalyzerWidget(QWidget *parent, DropletAnalyzer *analyzer);
    ~DropletAnalyzerWidget();
    void hide_plot_window();

public slots:
    void set_image_scale(double imageScale_um_per_px);
    void reset();
    void load_video_from_observation_widget(QString filePath, MicroJet jetSettings, double strobe_sweep_step_time_us);

signals:
    void show_frame(int frameNum);
    void print_to_output_window(QString s);
    void image_scaled_was_changed();

private:
    void setup();
    void magnification_was_edited();
    void image_scale_was_edited();
    void nozzle_diameter_was_changed();
    void strobe_step_time_was_changed(int stepTime_us);
    void set_frame_range(int numFrames);
    void update_view_settings();
    void video_loaded();
    void video_load_failed();
    void image_scale_detected();
    void image_scale_detection_failed();
    void load_video_button_pressed();
    void video_analysis_complete();
    void process_frame_request(int frameNum);

private:
    Ui::DropletAnalyzerWidget *ui {nullptr};
    ImageViewer *m_view {nullptr};
    DropletGraphWindow *m_graphWindow {nullptr};

    bool m_videoLoaded {false};
    std::vector<cv::Point> m_trackerPositions;

    DropletAnalyzer *m_analyzer {nullptr};
    std::unique_ptr<QThread> m_displayThread;
    std::vector<cv::Mat> m_video;
};

class DropletAnalyzerMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    DropletAnalyzerMainWindow(DropletAnalyzerWidget* analyzerWidget, QWidget *parent = nullptr);
    ~DropletAnalyzerMainWindow();

private:
    void closeEvent(QCloseEvent *bar) override;
    DropletAnalyzerWidget *m_dropletAnalyzerWidget {nullptr};
};


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
    QCustomPlot *m_plot;
};

#endif // DROPLETANALYZERWIDGET_H
