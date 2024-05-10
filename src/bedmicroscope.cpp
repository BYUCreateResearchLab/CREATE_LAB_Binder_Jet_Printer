#include "bedmicroscope.h"
#include <vector>

static int computeMedian(std::vector<uchar> elements)
{
    std::nth_element(elements.begin(),
                     elements.begin() + elements.size()/2,
                     elements.end());
    return elements[elements.size() / 2];
}

static cv::Mat compute_median(const std::vector<cv::Mat> &vec)
{
    // Note: Expects the image to be CV_8UC1
    cv::Mat medianImg(vec[0].rows, vec[0].cols, CV_8UC1);

    // Calculate median with parallel execution
    medianImg.forEach<uchar>([vec](uchar &p, const int *pos) -> void
    {
        std::vector<uchar> elements;
        elements.reserve(vec.size());
        for (int i{0}; i<vec.size(); ++i)
            elements.emplace_back(vec[i].at<uchar>(pos[0], pos[1]));
        p = computeMedian(elements);
    });

    return medianImg;
}

cv::Mat compute_mean(const std::vector<cv::Mat> &vec)
{
    // Note: Expects the images to be CV_8UC1
    cv::Mat meanImg(vec[0].rows, vec[0].cols, CV_8UC1, cv::Scalar(0));

    // Calculate mean with parallel execution
    meanImg.forEach<uchar>([&vec](uchar &p, const int *pos) -> void
    {
        int sum = 0;
        for (const auto &img : vec) {
            sum += img.at<uchar>(pos[0], pos[1]);
        }
        p = static_cast<uchar>(sum / vec.size());
    });

    return meanImg;
}

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
        QImage image(1920, 1080, QImage::Format_RGB888);
        image.fill(Qt::black);
        return image;
    }
}

void BedMicroscope::save_image(const QString &filename)
{
    // testing
    const int n = 5;

    std::vector<cv::Mat> frames;

    // Capture and store frames
    cv::Mat grayFrame;
    for (int i = 0; i < n; ++i)
    {
        cap >> frame; // error handling?
        cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        frames.push_back(grayFrame.clone());
    }

    cv::Mat meanFrame = compute_mean(frames);

//    cv::cvtColor(medianFrame, grayFrame, cv::COLOR_BGR2GRAY);

//    cap >> frame;
//    cv::Mat grayFrame;
//    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

    get_frame();
    cv::imwrite(filename.toStdString(), meanFrame);
}

#include "moc_bedmicroscope.cpp"
