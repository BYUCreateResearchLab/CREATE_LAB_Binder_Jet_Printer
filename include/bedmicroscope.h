#ifndef BEDMICROSCOPE_H
#define BEDMICROSCOPE_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <QImage>

class BedMicroscope : public QObject
{
    Q_OBJECT
public:
    explicit BedMicroscope(QObject *parent = nullptr);
    ~BedMicroscope();

    bool open_capture(int index);
    void disconnect_camera();
    QImage get_frame();
    bool is_connected() { return cap.isOpened(); }


private:
    cv::VideoCapture cap;
    cv::Mat frame;
};

#endif // BEDMICROSCOPE_H
