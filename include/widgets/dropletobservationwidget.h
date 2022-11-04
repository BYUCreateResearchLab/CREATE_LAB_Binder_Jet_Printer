#ifndef DROPLETOBSERVATIONWIDGET_H
#define DROPLETOBSERVATIONWIDGET_H

#include <QWidget>
#include "printerwidget.h"
#include <QMdiArea>
#include <ueye.h>
#include <camera.h>
#include <QTimer>

#include "jettingwidget.h"

class Camera;
class JetDrive;
class DropletAnalyzer;
class DropletAnalyzerWidget;
class QMainWindow;
class DropletAnalyzerMainWindow;
//class DropletAnalyzerWindow;

namespace Ui {
class DropletObservationWidget;
}

class DropletObservationWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit DropletObservationWidget(JetDrive *jetDrive, QWidget *parent = nullptr);
    ~DropletObservationWidget();
    void allow_widget_input(bool allowed) override;
    bool is_droplet_anlyzer_window_visible() const;

public slots:
    void jetting_was_turned_on();
    void jetting_was_turned_off();

    void show_droplet_analyzer_widget();
    void hide_droplet_analyzer_widget();

signals:
    void video_capture_complete();

private slots:
    void setup();
    void connect_to_camera();
    void set_settings();
    void capture_video();
    void add_frame_to_avi(ImageBufferPtr buffer);
    void stop_avi_capture();
    void camera_closed();
    void move_to_jetting_window();
    void move_towards_middle();
    //void strobe_sweep_button_clicked();
    void start_strobe_sweep();
    void update_strobe_sweep_offset();
    void trigger_jet_clicked();
    void framerate_changed();
    void exposure_changed();
    void save_video_clicked();
    void jet_for_three_minutes();
    void end_jet_timer();
    void update_progress_bar();

private:
    Ui::DropletObservationWidget *ui;
    HIDS m_cameraHandle {0};
    JetDrive *m_JetDrive {nullptr};
    Camera *m_Camera {nullptr};

    QTimer *m_JetVolumeTimer {nullptr};
    QTimer *m_ProgressBarTimer {nullptr};
    bool m_isJettingFor3Minutes {false};
    const int m_minutesToJet = 3;

    JettingWidget *m_JettingWidget {nullptr};

    std::unique_ptr<DropletAnalyzer> m_analyzer;
    DropletAnalyzerWidget *m_analyzerWidget {nullptr};
    std::unique_ptr<DropletAnalyzerMainWindow> m_analyzerWindow;

    int m_cameraFrameRate{};
    int m_numFramesToCapture{};

    int m_aviID{0};

    int m_numCapturedFrames{0};   // Keeps track of the current number of frames captured during video capture
    int m_currentStrobeOffset{-1}; // -1 means that a strobe sweep hasn't started

    int m_AOIWidth{680};

    bool m_isJetting {false};
    bool m_cameraIsConnected {false};
    bool m_videoHasBeenTaken {false};
    bool m_captureVideoWithSweep {false};

    QString m_tempFileName{};
};

#endif // DROPLETOBSERVATIONWIDGET_H
