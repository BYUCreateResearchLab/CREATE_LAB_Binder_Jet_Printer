#include "display.h"

#include <QDebug>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QBrush>
#include <QColor>
#include <QShowEvent>

Display::Display(QWidget *parent) : QWidget(parent)
{
    graphicsView = new GraphicsView;
    graphicsScene = new GraphicsScene;

    graphicsView->setScene(graphicsScene);
    graphicsView->setContentsMargins(0, 0, 0, 0);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(graphicsView);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
    connect(graphicsView, &GraphicsView::zoomFactorChanged, this, &Display::onZoomFactorChanged);
    connect(graphicsScene, &GraphicsScene::crossHairPosHChanged, this, &Display::crossHairHChanged);
    connect(graphicsScene, &GraphicsScene::crossHairPosVChanged, this, &Display::crossHairVChanged);
}

void Display::setImage(const QImage &image)
{
    if (displayOff)
        return;

    if (displayLimit && (limitTimer && !limitTimer->hasExpired(40)))
    {
        return;
    }

    QMutexLocker lock(&m_mutex);

    if (image.rect() != graphicsScene->sceneRect())
    {
        graphicsScene->setSceneRect(image.rect());

        if (fitInView)
        {
            graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
        }
        else
        {
            graphicsView->scale(zoomFactor);
        }
    }

    graphicsScene->setImage(image);

    // this is invoked from another thread so limitTimer cannot be created in constructor...
    delete limitTimer;
    limitTimer = new QElapsedTimer();
    limitTimer->start();
}

void Display::scale(double factor)
{
    zoomFactor = factor;
    graphicsView->scale(factor);
}

void Display::setFocusAOI(S_AUTOFOCUS_AOI aoi, bool visible)
{
    graphicsScene->updateFocusAOI(aoi, visible);
}

uint64_t Display::displayedFrames() const
{
    return graphicsScene->displayCount();
}

double Display::scaleFactor() const
{
    return zoomFactor;
}

void Display::fitToDisplay(bool fit)
{
    QMutexLocker lock(&m_mutex);

    fitInView = fit;
    if (fitInView)
    {
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
    }
    else
    {
        graphicsView->scale(zoomFactor);
    }
}

void Display::rotateRight()
{
    graphicsView->rotate(90);
}

void Display::rotateLeft()
{
    graphicsView->rotate(-90);
}

void Display::showCrosshair(bool visible)
{
    graphicsScene->showCrosshair(visible, visible);
}

void Display::showCrosshairVer(bool ver)
{
    graphicsScene->showCrosshair(ver, graphicsScene->crossHairHor());
}

void Display::showCrosshairHor(bool hor)
{
    graphicsScene->showCrosshair(graphicsScene->crossHairVer(), hor);
}

bool Display::isCrosshairVisible()
{
    return graphicsScene->isCrosshairVisible();
}

void Display::setDisplayLimit(bool enable)
{
    displayLimit = enable;
}

void Display::displayImage(bool enable)
{
    displayOff = enable;
}

void Display::resizeEvent(QResizeEvent *)
{
    if (fitInView)
    {
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void Display::showEvent(QShowEvent *)
{
    if (fitInView)
    {
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void Display::onZoomFactorChanged(double factor)
{
    emit zoomChanged(factor);
}

void Display::showHotpixel(const std::vector<Hotpixel>& hotpixels)
{
    graphicsScene->showHotpixel(hotpixels);
}

void Display::reset()
{
    graphicsScene->reset();
}

bool Display::isCrosshairHorVisible()
{
    return graphicsScene->crossHairHor();
}

bool Display::isCrosshairVerVisible()
{
    return graphicsScene->crossHairVer();
}

bool Display::isFitInView() const
{
    return fitInView;
}

bool Display::isDisplayOff() const
{
    return displayOff;
}

bool Display::isDisplayLimit() const
{
    return displayLimit;
}

void Display::setInfoText(const QString& text)
{
    graphicsScene->setText(text);
}

void Display::showInfotext(bool show)
{
    graphicsScene->showText(show);
}

bool Display::isInfoTextVisible()
{
    return graphicsScene->isInfoTextVisible();
}

void Display::saveCurrentFrame(const QString& filename) const
{
    auto img = graphicsView->grab();
    img.save(filename);
}

Display::~Display()
{
    delete limitTimer;
}

#include "moc_display.cpp"
