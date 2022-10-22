#include <graphicsview.h>
#include <QDebug>
#include <QWheelEvent>


void GraphicsView::zoomIn()
{
    scale(scale_factor + 0.25);
}

void GraphicsView::zoomOut()
{
    scale(scale_factor - 0.25);
}

void GraphicsView::scale(double factor)
{
    if (factor <= 0.25)
    {
        factor = 0.25;
    }
    else if (factor > 4)
    {
        factor = 4;
    }

    resetTransform();
    QGraphicsView::scale(factor, factor);
    scale_factor = factor;

    emit zoomFactorChanged(factor);

}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        if (event->angleDelta().y() < 0)
        {
            zoomOut();
        }
        else if (event->angleDelta().y() > 0)
        {
            zoomIn();
        }

         return;
    }

   QGraphicsView::wheelEvent(event);
}

double GraphicsView::scaleFactor() const
{
    return scale_factor;
}

#include "moc_graphicsview.cpp"
