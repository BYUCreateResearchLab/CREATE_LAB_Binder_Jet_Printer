#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QObject>
#include <QImage>
#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMutex>
#include <QElapsedTimer>
#include <QWheelEvent>

class DAGraphicsView;
class DAGraphicsScene;

class ImageViewer : public QWidget
{
   Q_OBJECT
public:
   explicit ImageViewer(QWidget* parent = nullptr);
   ~ImageViewer();

private:
   QMutex m_mutex;
public:
   void setImage(const QImage& image);
   void scale(double factor);
   double scaleFactor() const;
   void fitToDisplay(bool fit);
   void displayImage(bool enable);
   void setDisplayLimit(bool enable);

   void reset();

   void rotateRight();
   void rotateLeft();

   uint64_t displayedFrames() const;

   bool isFitInView() const;
   bool isDisplayOff() const;
   bool isDisplayLimit() const;

   void saveCurrentFrame(const QString& filename) const;

protected:
   void resizeEvent(QResizeEvent*) override;
   void showEvent(QShowEvent*) override;

   DAGraphicsView *graphicsView;
   DAGraphicsScene *graphicsScene;

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

#endif // IMAGEVIEWER_H
