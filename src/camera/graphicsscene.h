#pragma once

#include "utils.h"
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <ueye.h>

enum class HotpixelType { User, Factory, Software };

struct Hotpixel
{
    uint32_t x;
    uint32_t y;
    HotpixelType type;

    inline bool operator == (const Hotpixel &lhs) const
    {
        return ((lhs.x==x) && (lhs.y==y)/* && (lhs.type == type)*/);
    }
};

typedef std::vector<Hotpixel> HotpixelList;

class GraphicsHotpixelItem : public QGraphicsRectItem
{
public:
    explicit GraphicsHotpixelItem(HotpixelType type, QGraphicsItem *parent = nullptr) :
        QGraphicsRectItem(parent)
    {
        setHotpixelType(type);
    }

    void setHotpixelType(HotpixelType type)
    {
        m_HotpixelType = type;

        QPen pen;
        switch(type)
        {
        case HotpixelType::User:
            pen.setColor(QColor(255, 140, 0));
            break;

        case HotpixelType::Factory:
            pen.setColor(QColor(255, 0, 0));
            break;

        case HotpixelType::Software:
            pen.setColor(QColor(0, 255, 0));
            break;
        }

        pen.setWidthF(0.5);
        setPen(pen);
    }

    HotpixelType hotpixelType() const
    {
        return m_HotpixelType;
    }

private:
    HotpixelType m_HotpixelType{};

   // enum { Type = UserType + 1 };
};

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit GraphicsScene(QObject *parent = nullptr);
    void setImage(const QImage& image);
    void showHotpixel(const std::vector<Hotpixel>& vecHotpixel);

    void showCrosshair(bool ver, bool hor);
    void setCrosshairPos(QPointF pos);
    void showText(bool enabled);

    void reset();

    NO_DISCARD bool isCrosshairVisible() const;
    void updateFocusAOI(S_AUTOFOCUS_AOI aoi, bool visible);

    NO_DISCARD bool crossHairHor() const;
    NO_DISCARD bool crossHairVer() const;
    NO_DISCARD bool isInfoTextVisible() const;

    uint64_t displayCount();

public slots:
    void setText(const QString& text);

protected:
    void drawBackground(QPainter *painter, const QRectF& rect) override;
    void initCrosshair();
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    bool crosshairVerVisible = false;
    bool crosshairHorVisible = false;
    bool crosshairFollowMouseMove = true;
    bool textVisible = false;

    QImage image;
    QGraphicsLineItem *lineHorizontal{};
    QGraphicsLineItem *lineVertical{};
    QMap<int, QGraphicsRectItem*> focusRects;
    QGraphicsTextItem* infotext;
    QMutex m_mutex;
    uint64_t m_nFrameDisplayCount{0};
    uint64_t m_invokeCnt{0};

    void setCrosshairPosH(QPointF pos);
    void setCrosshairPosV(QPointF pos);

signals:
    void crossHairPosHChanged(int y);
    void crossHairPosVChanged(int x);
};
