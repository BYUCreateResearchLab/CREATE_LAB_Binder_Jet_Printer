#include "imageviewer.h"
#include <QDebug>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QBrush>
#include <QColor>
#include <QShowEvent>
#include <QThread>
#include <QMutexLocker>

class DAGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    void zoomIn() { scale(scale_factor + 0.25); }
    void zoomOut() { scale(scale_factor - 0.25); }

    void scale(double factor)
    {
        if (factor <= 0.25) factor = 0.25;
        else if (factor > 4) factor = 4;

        resetTransform();
        QGraphicsView::scale(factor, factor);
        scale_factor = factor;

        emit zoomFactorChanged(factor);
    }

    double scaleFactor() const { return scale_factor; }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->modifiers().testFlag(Qt::ControlModifier))
        {
            if (event->angleDelta().y() < 0) zoomOut();
            else if (event->angleDelta().y() > 0) zoomIn();

            return;
        }

        QGraphicsView::wheelEvent(event);
    }

signals:
    void zoomFactorChanged(double factor);

private:
    QImage image;
    double scale_factor = 1.0;
};



class DAGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit DAGraphicsScene(QObject *parent = nullptr) :
        QGraphicsScene(parent)
    {
        reset();
    }

    void setImage(const QImage& image)
    {
        if (m_mutex.tryLock())
        {
            this->image = image;
            m_mutex.unlock();
            m_invokeCnt++;
            update();
        }
    }

    void reset()
    {
        clear();
        image = QImage();
        setBackgroundBrush(QColor(240,240,240));
        update(sceneRect());
    }

    uint64_t displayCount() { return m_nFrameDisplayCount; }

protected:
    void drawBackground(QPainter *painter, const QRectF& rect) override
    {
        QGraphicsScene::drawBackground(painter, rect);

        QMutexLocker lock(&m_mutex);
        painter->drawImage(0, 0, image);

        if (m_nFrameDisplayCount < m_invokeCnt) { m_nFrameDisplayCount++; }
    }

private:
    QImage image;
    QMutex m_mutex;
    uint64_t m_nFrameDisplayCount {0};
    uint64_t m_invokeCnt {0};
};



ImageViewer::ImageViewer(QWidget *parent) : QWidget(parent)
{
    graphicsView = new DAGraphicsView;
    graphicsScene = new DAGraphicsScene;

    graphicsView->setScene(graphicsScene);
    graphicsView->setContentsMargins(0, 0, 0, 0);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(graphicsView);
    layout->setContentsMargins(0, 0, 0, 0);
    graphicsView->setStyleSheet("border-color: rgb(220, 220, 220);border-width : 1.0px;border-style:solid;");

    setLayout(layout);
    connect(graphicsView, &DAGraphicsView::zoomFactorChanged, this, &ImageViewer::onZoomFactorChanged);
}

ImageViewer::~ImageViewer()
{
    //qDebug() << __FUNCTION__ << "reallocations" << m_track.reallocs;
    delete limitTimer;
}

void ImageViewer::setImage(const QImage &image)
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

    //qDebug() << QThread::currentThread();
}

void ImageViewer::scale(double factor)
{
    zoomFactor = factor;
    graphicsView->scale(factor);
}

uint64_t ImageViewer::displayedFrames() const
{
    return graphicsScene->displayCount();
}

double ImageViewer::scaleFactor() const
{
    return zoomFactor;
}

void ImageViewer::fitToDisplay(bool fit)
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

void ImageViewer::rotateRight()
{
    graphicsView->rotate(90);
}

void ImageViewer::rotateLeft()
{
    graphicsView->rotate(-90);
}

void ImageViewer::setDisplayLimit(bool enable) { displayLimit = enable; }

void ImageViewer::displayImage(bool enable) { displayOff = enable; }

void ImageViewer::resizeEvent(QResizeEvent *)
{
    if (fitInView)
    {
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void ImageViewer::showEvent(QShowEvent *)
{
    if (fitInView)
    {
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void ImageViewer::onZoomFactorChanged(double factor) { emit zoomChanged(factor); }

void ImageViewer::reset() { graphicsScene->reset(); }

bool ImageViewer::isFitInView() const { return fitInView; }

bool ImageViewer::isDisplayOff() const { return displayOff; }

bool ImageViewer::isDisplayLimit() const { return displayLimit; }

void ImageViewer::saveCurrentFrame(const QString& filename) const
{
    auto img = graphicsView->grab();
    img.save(filename);
}

#include "imageviewer.moc" // include .moc file for DAGraphicsScene/View QOBJECTS declared here
#include "moc_imageviewer.cpp" // include moc_.cpp file to have it not part of the mocs_compilation.cpp
