#pragma once

#include <QWidget>
#include <QElapsedTimer>
#include <ueye.h>

#include "graphicsscene.h"
#include "graphicsview.h"
#include "utils.h"

class Display : public QWidget
{
    Q_OBJECT

    QMutex m_mutex;
public:
    explicit Display(QWidget* parent);
    ~Display();
    void setImage(const QImage& image);
    void scale(double factor);
    NO_DISCARD double scaleFactor() const;
    void fitToDisplay(bool fit);
    void displayImage(bool enable);
    void setDisplayLimit(bool enable);
    void showHotpixel(const std::vector<Hotpixel>& hotpixels);

    void showCrosshair(bool visible);
    void showCrosshairVer(bool ver);
    void showCrosshairHor(bool hor);

    NO_DISCARD bool isCrosshairVisible();
    NO_DISCARD bool isInfoTextVisible();

    void setInfoText(const QString& text);
    void showInfotext(bool show);

    void reset();

    void rotateRight();
    void rotateLeft();

    NO_DISCARD uint64_t displayedFrames() const;

    void setFocusAOI(S_AUTOFOCUS_AOI aoi, bool visible);
    NO_DISCARD bool isCrosshairHorVisible();

    NO_DISCARD bool isCrosshairVerVisible();

    NO_DISCARD bool isFitInView() const;
    NO_DISCARD bool isDisplayOff() const;
    NO_DISCARD bool isDisplayLimit() const;

    void saveCurrentFrame(const QString& filename) const;

protected:
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent*) override;

    GraphicsView *graphicsView;
    GraphicsScene *graphicsScene;

    double zoomFactor = 1.;
    bool fitInView = false;
    bool displayOff = false;
    bool displayLimit = false;

    QElapsedTimer* limitTimer = nullptr;

private slots:
    void onZoomFactorChanged(double factor);


signals:
    void zoomChanged(double zoomFactor);

    void crossHairHChanged(int y);
    void crossHairVChanged(int x);
};
