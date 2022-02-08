#pragma once
#include "utils.h"
#include <QGraphicsView>

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    void zoomIn();
    void zoomOut();

    void scale(double factor);
    NO_DISCARD double scaleFactor() const;

protected:
    void wheelEvent(QWheelEvent *event) override;

signals:
    void zoomFactorChanged(double factor);

private:
    QImage image;
    double scale_factor = 1.0;
};
