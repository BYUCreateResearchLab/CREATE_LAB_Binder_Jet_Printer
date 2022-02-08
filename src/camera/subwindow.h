#ifndef SUBWINDOW_H
#define SUBWINDOW_H

#include <QMdiSubWindow>
#include <QScrollArea>
#include <QThread>
#include <QProgressDialog>
#include "graphicsview.h"
#include "graphicsscene.h"
#include <ueye.h>
#include "display.h"
#include "camera.h"

class SubWindow : public QMdiSubWindow
{
    Q_OBJECT
public:
    explicit SubWindow(const UEYE_CAMERA_INFO& info, QWidget *parent = nullptr);

    NO_DISCARD Display* display();
    NO_DISCARD QSharedPointer<Camera> camera();

signals:
    void cameraOpenFinished();
    void updateDisplay(const QImage& image);
    void updateImageInfo(const UEYEIMAGEINFO& image_info);

protected slots:
    void onCameraOpenFinished();

public slots:
    void onParameterLoadFromEeprom();
    void onParameterLoadFromFile();
    void onParameterSaveToEeprom();
    void onParameterSaveToFile();
    void onImageSaveWithAnnotations();

    void onCaptureFreerun(bool checked);
    void onSnapshotFreerun(bool checked);
    void onCaptureTriggered(bool checked);
    void onSnapshotTriggered(bool checked);

    void onImageLoad();
    void onImageSave();

    void onFrameReceived(ImageBufferPtr buffer);
    void setMarkHotpixel(bool checked);

    void onResetCamera();
    void setFocusAOI(AUTOFOCUS_AOI aoi, bool visible);


protected:
    void closeEvent(QCloseEvent *closeEvent) override;
    void resizeEvent(QResizeEvent* resizeEvent) override;
    void enterEvent(QEvent* event) override;

    void fixMaximizedMenu();

protected slots:
    void onUpdateDisplay(const QImage& image);
    void onUpdateImageInfo(const UEYEIMAGEINFO& image_info);

private:
    class InitCameraThread;

    Display *m_pDisplayWidget;
    InitCameraThread *m_pInitCamThread;

    QSharedPointer<Camera> m_camera;
    QProgressDialog* m_dialog;

};

#endif // SUBWINDOW_H
