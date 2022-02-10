#ifndef DROPLETOBSERVATIONWIDGET_H
#define DROPLETOBSERVATIONWIDGET_H

#include <QWidget>
#include "printerwidget.h"
#include <QMdiArea>
#include <ueye.h>
#include <camera.h>

class Camera;

namespace Ui {
class DropletObservationWidget;
}

class DropletObservationWidget : public PrinterWidget
{
    Q_OBJECT

public:
    explicit DropletObservationWidget(QWidget *parent = nullptr);
    ~DropletObservationWidget();
    void allow_widget_input(bool allowed) override;

private slots:
    void connect_to_camera();
    void set_settings();
    void capture_video();
    void add_frame_to_avi(ImageBufferPtr buffer);
    void stop_avi_capture();

private:
    Ui::DropletObservationWidget *ui;
    HIDS mCameraHandle{0};
    Camera *mCamera{nullptr};
    int mNumCapturedFrames{0};
    int mNumFramesToCapture{10};
    int mAviID{0};
};

#endif // DROPLETOBSERVATIONWIDGET_H
