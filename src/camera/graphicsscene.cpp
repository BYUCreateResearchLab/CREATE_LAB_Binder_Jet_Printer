#include "graphicsscene.h"
#include <QDebug>
#include <QMutexLocker>

GraphicsScene::GraphicsScene(QObject *parent) :
    QGraphicsScene(parent)
{

    reset();

    initCrosshair();

    infotext = new QGraphicsTextItem;
    infotext->setPos(10,10);
    addItem(infotext);
    infotext->setVisible(textVisible);
}

void GraphicsScene::initCrosshair()
{
    QPen pen;

    pen.setColor(QColor(Qt::red));
    pen.setWidthF(0.2);

    lineHorizontal = addLine(QLineF(0, 0, 0, 0), pen);
    lineVertical = addLine(QLineF(0, 0, 0, 0), pen);
    showCrosshair(false, false);
}

void GraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);

    QMutexLocker lock(&m_mutex);
    painter->drawImage(0, 0, image);

    if (m_nFrameDisplayCount < m_invokeCnt)
    {
        m_nFrameDisplayCount++;
    }
}

void GraphicsScene::setImage(const QImage &image)
{
    if (m_mutex.tryLock())
    {
        this->image = image;
        m_mutex.unlock();
        m_invokeCnt++;
        update();
    }
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pos = event->scenePos();

    if (crosshairFollowMouseMove && crosshairHorVisible)
    {
        setCrosshairPosH(pos);
    }
    if (crosshairFollowMouseMove && crosshairVerVisible)
    {
        setCrosshairPosV(pos);
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (crosshairHorVisible || crosshairVerVisible)
    {
        crosshairFollowMouseMove = !crosshairFollowMouseMove;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void GraphicsScene::showHotpixel(const std::vector<Hotpixel>& vecHotpixel)
{
    /* remove old items */
    for (auto* item : items())
    {
        if (item != lineHorizontal && item != lineVertical)
        {
            removeItem(item);
        }
    }

    for (auto& hotpixel : vecHotpixel)
    {
        auto *item = new GraphicsHotpixelItem(hotpixel.type);

        item->setPos(0, 0);
        item->setRect(0, 0, 3, 3);
        item->moveBy(hotpixel.x - 1, hotpixel.y - 1);
        //item->setFlag(QGraphicsItem::ItemIsSelectable, true);

        addItem(item);
    }
}

void GraphicsScene::showCrosshair(bool ver, bool hor)
{
    lineHorizontal->setVisible(hor);
    lineVertical->setVisible(ver);

    setCrosshairPos(QPointF(image.size().width() / 2.0, image.height() / 2.0));
    crosshairHorVisible = hor;
    crosshairVerVisible = ver;
}

void GraphicsScene::setCrosshairPos(QPointF pos)
{
    setCrosshairPosH(pos);
    setCrosshairPosV(pos);
}

bool GraphicsScene::isCrosshairVisible() const
{
    return crosshairHorVisible && crosshairVerVisible;
}

void GraphicsScene::reset()
{
    clear();
    image = QImage();

    setBackgroundBrush(Qt::gray);
    initCrosshair();

    update(sceneRect());
}

void GraphicsScene::updateFocusAOI(S_AUTOFOCUS_AOI aoi, bool visible)
{
    QPen pen;

    pen.setColor(QColor(Qt::green));
    pen.setWidthF(4);

    QRectF newPos(aoi.rcAOI.s32X, aoi.rcAOI.s32Y, aoi.rcAOI.s32Width, aoi.rcAOI.s32Height);
    auto nAoi = static_cast<int>(aoi.uNumberAOI);
    if (focusRects.contains(nAoi))
    {
        if (focusRects[nAoi]->rect() != newPos)
        {
            focusRects[nAoi]->setRect(newPos);
        }
    }
    else
    {
        focusRects[nAoi] = addRect(newPos, pen);
    }

    focusRects[nAoi]->setVisible(visible);
}

void GraphicsScene::setCrosshairPosH(QPointF pos)
{
    lineHorizontal->setLine(0, pos.y(), sceneRect().width(), pos.y());
    if (crosshairHorVisible)
    {
        auto y = static_cast<int>(lineHorizontal->line().y1());
        emit crossHairPosHChanged(y);
    }
}

void GraphicsScene::setCrosshairPosV(QPointF pos)
{
    lineVertical->setLine(pos.x(), 0, pos.x(), sceneRect().height());
    if (crosshairVerVisible)
    {
        auto x = static_cast<int>(lineVertical->line().x1());
        emit crossHairPosVChanged(x);
    }
}

bool GraphicsScene::crossHairHor() const
{
    return crosshairHorVisible;
}

bool GraphicsScene::crossHairVer() const
{
    return crosshairVerVisible;
}

void GraphicsScene::setText(const QString& text)
{
    infotext->setHtml(text);
}

void GraphicsScene::showText(bool enabled)
{
    textVisible = enabled;
    infotext->setVisible(textVisible);
    if(!enabled)
    {
        infotext->setHtml("");
    }
    update();
}

bool GraphicsScene::isInfoTextVisible() const
{
    return textVisible;
}

uint64_t GraphicsScene::displayCount()
{
    return m_nFrameDisplayCount;
}
