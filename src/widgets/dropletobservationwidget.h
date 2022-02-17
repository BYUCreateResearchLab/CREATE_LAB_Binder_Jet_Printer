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

public slots:
    void jetting_was_turned_on();
    void jetting_was_turned_off();

signals:
    void video_capture_complete();

private slots:
    void connect_to_camera();
    void set_settings();
    void capture_video();
    void add_frame_to_avi(ImageBufferPtr buffer);
    void stop_avi_capture();
    void camera_closed();
    void move_to_jetting_window();
    void strobe_sweep_button_clicked();
    void start_strobe_sweep();
    void update_strobe_sweep_offset();
    void trigger_jet_clicked();
    void framerate_changed();
    void exposure_changed();
    void save_video_clicked();

private:
    Ui::DropletObservationWidget *ui;
    HIDS mCameraHandle{0};
    JetDrive *mJetDrive{nullptr};
    Camera *mCamera{nullptr};
    QTimer *mSweepTimer{nullptr};
    JettingWidget *mJettingWidget{nullptr};

    int mCameraFrameRate{};
    int mNumFramesToCapture{};

    int mAviID{0};

    int mNumCapturedFrames{0};   // Keeps track of the current number of frames captured during video capture
    int mCurrentStrobeOffset{-1}; // -1 means that a strobe sweep hasn't started

    int AOIWidth{680};

    bool mIsJetting{false};
    bool mCameraIsConnected{false};
    bool mVideoHasBeenTaken{false};
    bool mCaptureVideoWithSweep{false};

    QString mTempFileName{};
};

#endif // DROPLETOBSERVATIONWIDGET_H
