#include "bedmicroscope.h"

BedMicroscope::BedMicroscope(QObject *parent) :
    QObject(parent)
{

}

BedMicroscope::~BedMicroscope()
{
    if (cap.isOpened())
    {
        cap.release();
    }

}

bool BedMicroscope::open_capture(int index)
{
    cap.open(index);
    return cap.isOpened();
}

void BedMicroscope::disconnect_camera()
{
    cap.release();
}

QImage BedMicroscope::get_frame()
{
    cap >> frame; // Capture frame from camera

    if (!frame.empty())
    {
        // Convert OpenCV Mat to QImage
        QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);
        return img; // why copy here?
    }
    else // if could not get frame
    {
        QImage image(640, 480, QImage::Format_RGB888);
        image.fill(Qt::black);
        return image;
    }
}
