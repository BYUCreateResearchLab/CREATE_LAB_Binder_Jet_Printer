#include "dropletanalyzer.h"
#include <QDebug>
#include <QTimerEvent>
#include <iostream>
#include "linearanalysis.h"

#include <opencv2/opencv.hpp>

// ====================================================
// FUNCTIONS FOR GETTING THE MEDIAN FRAME

int computeMedian(std::vector<uchar> elements)
{
    std::nth_element(elements.begin(),
                     elements.begin() + elements.size()/2,
                     elements.end());
    return elements[elements.size() / 2];
}

cv::Mat compute_median(std::vector<cv::Mat> vec)
{
    // Note: Expects the image to be CV_8UC1
    cv::Mat medianImg(vec[0].rows, vec[0].cols, CV_8UC1);
    // only choose subset of frames for median (don't need all for a good median)
    const int frameIncrement = std::ceil(vec.size() / 15.0);

    for (int row{0}; row < vec[0].rows; row++) // for each row
    {
        for (int col{0}; col < vec[0].cols; col++) // for each column
        {
            // get pixels from every nth frame (defined by frameIncrement)
            std::vector<uchar> elements;
            for (size_t imgNumber{0}; imgNumber<vec.size(); imgNumber+=frameIncrement)
            {
                elements.push_back(vec[imgNumber].at<uchar>(row, col));
            }

            // get median pixel
            medianImg.at<uchar>(row, col) = computeMedian(elements);
        }
    }
    return medianImg;
}

// ====================================================================

DropletAnalyzer::DropletAnalyzer() : QObject()
{
    m_thread.reset(new QThread);
    m_thread->setObjectName("Converter Thread");
    camera_settings().cameraPixelSize_um = 5.5; // for IDS camera
    m_jetSettings.reset();
    moveToThread(m_thread.get());
    m_thread->start();
}

DropletAnalyzer::~DropletAnalyzer()
{
    QMetaObject::invokeMethod(this, "cleanup");
    m_thread->wait();
}

QImage DropletAnalyzer::image() const { return m_image; }

DropletCameraSettings& DropletAnalyzer::camera_settings()
{
    QMutexLocker lock(&m_mutex);
    return m_cameraSettings;
}

void DropletAnalyzer::load_video(const std::string& filename)
{
    // don't call reset here
    auto *m_videoCapture = new cv::VideoCapture(filename);
    // load all frames into ram
    if (m_videoCapture->isOpened())
    {
        m_numFrames = m_videoCapture->get(cv::CAP_PROP_FRAME_COUNT);
        while (1)
        {
            cv::Mat frame;
            if (!m_videoCapture->read(frame)) break;
            cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
            m_video.push_back(frame);
        }
        emit video_loaded();
    }
    else emit video_load_failed();

    m_videoCapture->release();
    delete m_videoCapture;
}

int DropletAnalyzer::get_number_of_frames()
{
    QMutexLocker lock(&m_mutex);
    if (is_video_loaded()) return m_numFrames;
    else return 0;
}

void DropletAnalyzer::show_frame(int frameNum)
{    
    if (frameNum >= m_numFrames)
    {
        qDebug() << "Frame is out of range";
        return;
    }

    m_frameNum = frameNum;

    // process frame here
    cv::Mat frame {m_video[m_frameNum].clone()};

    int thresholdValue {40};

    if (m_viewSettings.showProcessedFrame)
    {
        // calculate absolute difference of current frame and the median frame
        cv::absdiff(frame, m_medianFrame, frame);
        // calculate threshold
        cv::threshold(frame, frame, thresholdValue, 255, cv::THRESH_BINARY);
    }

    // make sure mat is converted to RGB
    cv::cvtColor(frame, frame, cv::COLOR_GRAY2RGB);
    Q_ASSERT(frame.type() == CV_8UC3);

    if (m_viewSettings.showNozzleOutline)
    {
        if (m_nozzleOutline.size() >= 1)
        {
            cv::polylines(frame, m_nozzleOutline, true, cv::Scalar(150,150,150), 2);
            // get bottom face
            std::vector<cv::Point> nozzleTip = {m_nozzleOutline[1], m_nozzleOutline[2]};
            cv::polylines(frame, nozzleTip, true, cv::Scalar(0,0,256), 4);


        }
        if (m_originPoint != cv::Point(0,0))
            cv::drawMarker(frame, m_originPoint, cv::Scalar(255,255,255), cv::MARKER_CROSS, 20, 3);
    }

    if (m_viewSettings.showDropletContour)
    {
        const int lineThickness = 2;
        const cv::Scalar color {256,0,0};
        const auto& contours = m_dropletContours[m_frameNum];
        for (size_t i=0; i < contours.size(); i++)
        {
            cv::drawContours(frame, contours, (int)i, color, lineThickness);
        }
    }

    if (m_viewSettings.showTracking)
    {
        cv::drawMarker(frame, m_trackerPoints[m_frameNum], cv::Scalar(0,255,0), cv::MARKER_CROSS, 20, 3);
    }

    const int w = frame.cols;
    const int h = frame.rows;
    if (m_image.size() != QSize{w, h})
        m_image = QImage(w, h, QImage::Format_RGB888);
    cv::Mat mat(h, w, CV_8UC3, m_image.bits(), m_image.bytesPerLine());
    cv::resize(frame, mat, mat.size(), 0, 0, cv::INTER_AREA);
    emit image_ready(m_image);
}

void DropletAnalyzer::reset()
{
    m_medianFrame.release();
    m_video.clear();
    m_originPoint = cv::Point(0,0);
    m_nozzleOutline.clear();
    m_dropletContours.clear();
    m_trackerPoints.clear();
    m_trackingData.clear();
    m_jetSettings.reset();
}

void DropletAnalyzer::update_view_settings(const DropViewSettings& viewSettings)
{
    QMutexLocker lock(&m_mutex);
    m_viewSettings = viewSettings;
    //qDebug() << QThread::currentThread();
}

void DropletAnalyzer::cleanup()
{
    m_thread->quit();
}

bool DropletAnalyzer::is_video_loaded()
{
    return !m_video.empty();
}

cv::Point DropletAnalyzer::calculate_default_droplet_origin()
{
        std::vector<double> xPoints, yPoints;
        xPoints.reserve(m_trackerPoints.size());
        yPoints.reserve(m_trackerPoints.size());
        for (const auto& point : m_trackerPoints)
        {
            if (point == m_noTrackPoint) continue;
            xPoints.push_back(point.x);
            yPoints.push_back(point.y);
        }
        const double intercept = LinearAnalysis::find_fit_line(yPoints, xPoints).intercept;
        return cv::Point(intercept, 0);
}

void DropletAnalyzer::filter_data_by_residuals(double threshold)
{
    auto fitLine = LinearAnalysis::find_fit_line(m_trackingData.t, m_trackingData.y);
    auto residuals = LinearAnalysis::calculate_residuals(fitLine, m_trackingData.t, m_trackingData.y);
    int i {0};
    for (size_t s{0}; s < residuals.size(); s++)
    {
        if (std::abs(residuals[i]) > threshold)
        {
            m_trackingData.x.erase(std::next(m_trackingData.x.begin(), i));
            m_trackingData.y.erase(std::next(m_trackingData.y.begin(), i));
            m_trackingData.t.erase(std::next(m_trackingData.t.begin(), i));
            residuals.erase(std::next(residuals.begin(), i));
        }
        else i++;
    }
}

void DropletAnalyzer::rotate_data_by_angle(double angle_rad)
{
    m_trackingData.drop_angle_rad = angle_rad;
    for (int i{0}; i<m_trackingData.x.size(); i++)
    {
        const double yNew = (m_trackingData.y[i] * std::cos(angle_rad)) +
                            (m_trackingData.x[i] * std::sin(angle_rad));
        const double xNew = (m_trackingData.y[i] * -std::sin(angle_rad)) +
                            (m_trackingData.x[i] * std::cos(angle_rad));

        m_trackingData.y[i] = yNew;
        m_trackingData.x[i] = xNew;
    }
}

void DropletAnalyzer::calculate_droplet_velocity()
{
    LinearAnalysis::FitLine fitLine = LinearAnalysis::find_fit_line(m_trackingData.t, m_trackingData.y);
    m_trackingData.velocity_m_s = fitLine.slope;
    m_trackingData.intercept = fitLine.intercept;
}

void DropletAnalyzer::analyze_video()
{
    if (m_medianFrame.empty()) calculate_median_frame();
    if (m_nozzleOutline.empty()) detect_nozzle();
    if (m_dropletContours.empty()) detect_contours();
    if (m_trackerPoints.empty()) track_droplet();

    // TODO: Instead of just using center of nozzle for origin,
    // find the intersection between the droplet line and the nozzle line
    // for the origin point
    // set origin point if the nozzle was not detected
    if (m_originPoint == cv::Point(0,0))
        m_originPoint = calculate_default_droplet_origin();


    calculate_scaled_drop_pos();
    calculate_droplet_velocity();

    //generate_tracking_csv();

    emit video_analysis_successful();
}

void DropletAnalyzer::calculate_median_frame()
{
    m_medianFrame = compute_median(m_video);
}

Points2D filter_contours(const Points2D &unfilteredContours,
                                                int minContourSize)
{
    Points2D contours;
    // only include large enough contours
    for (int i=0; i < unfilteredContours.size(); i++)
    {
        if (cv::contourArea(unfilteredContours[i]) >= minContourSize)
        {
            contours.push_back(unfilteredContours[i]);
        }
    }
    return contours;
}

Points2D sort_contours(Points2D contours)
{
    // sort each contour by increasing y value and then by increasing x value
    for (int i=0; i < contours.size(); i++)
    {
        std::sort(contours[i].begin(), contours[i].end(),
            [](const cv::Point &a, const cv::Point &b) -> bool
        {
            if (a.y != b.y) return a.y < b.y;
            else return a.x < b.x;
        });
    }
    return contours;
}

void DropletAnalyzer::detect_contours()
{
    for (auto& frame : m_video)
    {
        cv::Mat processedFrame;
        Points2D unfilteredContours;
        std::vector<cv::Vec4i> hierarchy;

        // TODO: update the threshold filter here and on display to be adaptive
        // in some way instead of just hardcoding in the threshold.
        // Also make sure that the threshold is done the same in both places by
        // creating a function used in both places
        const int thresholdValue = 40;
        int minContourSize = 100; // in pixels

        // process image for analysis
        processedFrame = frame.clone();
        cv::absdiff(processedFrame, m_medianFrame, processedFrame);
        cv::threshold(processedFrame, processedFrame, thresholdValue, 255, cv::THRESH_BINARY);

        cv::findContours(processedFrame, unfilteredContours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
        Points2D contours = filter_contours(unfilteredContours, minContourSize);

        m_dropletContours.push_back(contours);
    }
}

void DropletAnalyzer::track_droplet()
{
    for (auto& contours : m_dropletContours)
    {
        if (contours.empty())
        {
            m_trackerPoints.push_back(m_noTrackPoint); // position if tracking not succesful
            continue;
        }
        else
        {
            auto sorted = sort_contours(contours);

            auto comparison = [](const cv::Point& a, const cv::Point& b)
            {
                return a.y < b.y;
            };

            auto max = std::max_element(sorted[0].begin(), sorted[0].end(), comparison);
            // get average if there is more than one max y pixel
            if (max == sorted[0].end()) m_trackerPoints.push_back(*max);
            else
            {
                m_trackerPoints.push_back(cv::Point((max->x + sorted[0].back().x) / 2.0 , max->y));
            }
        }
    }
}

void DropletAnalyzer::calculate_scaled_drop_pos()
{
    m_trackingData.clear();
    for (int frame {0}; frame < m_trackerPoints.size(); frame++)
    {
        const auto& point = m_trackerPoints[frame];
        if (point == m_noTrackPoint) continue;
        if (point.y < m_originPoint.y) continue; // don't include points above the nozzle
        m_trackingData.x.push_back((point.x - m_originPoint.x) * m_cameraSettings.imagePixelSize_um);
        m_trackingData.y.push_back((point.y - m_originPoint.y) * m_cameraSettings.imagePixelSize_um);
        m_trackingData.t.push_back(frame * m_strobeStepTime_us);
    }

    // filter by residuals of fit line
    filter_data_by_residuals(500);

    // correct for rotation here
    const LinearAnalysis::FitLine fitLine = LinearAnalysis::find_fit_line(m_trackingData.y, m_trackingData.x);
    const double jettingAngle_rad = std::atan(fitLine.slope);
    rotate_data_by_angle(jettingAngle_rad);
}

void DropletAnalyzer::generate_tracking_csv()
{
    for (int i {0}; i < m_trackingData.x.size(); i++)
    {
        qDebug() << m_trackingData.x[i] << "," << m_trackingData.y[i] << "," << m_trackingData.t[i];
    }
}

void DropletAnalyzer::set_jetting_settings(const MicroJet &jetSettings)
{
    QMutexLocker lock(&m_mutex);
    m_jetSettings = jetSettings;
}

std::optional<MicroJet> DropletAnalyzer::get_jetting_settings()
{
    QMutexLocker lock(&m_mutex);
    return m_jetSettings;
}

void DropletAnalyzer::detect_nozzle()
{
    cv::Mat bw;
    cv::GaussianBlur(m_medianFrame, bw, cv::Size(5,5), 0); // first, blur the image
    // calculate the threshold value from the mean pixel value
    const float meanVal = cv::mean(m_medianFrame).val[0];
    cv::threshold(bw, bw, meanVal*0.5, 255, cv::THRESH_BINARY_INV);
    // THRESH_OTSU will automatically calculate the appropriate threshold value
    // But doesn't do a very good job when the nozzle is not in frame...
    //const double threshTest = cv::threshold(bw, bw, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // since sometimes after thresholding reflections on the nozzle exceed the threshold:
    // Floodfill from middle of the image so that everything is white except for inside the nozzle
    cv::Mat im_floodfill = bw.clone();
    cv::floodFill(im_floodfill, cv::Point(im_floodfill.cols/2,im_floodfill.rows/2), cv::Scalar(255));

    // Invert floodfilled image
    cv::Mat im_floodfill_inv;
    cv::bitwise_not(im_floodfill, im_floodfill_inv);

    // Combine the two images to get solid filled in threshold of the nozzle
    cv::Mat filled = (bw | im_floodfill_inv);

    // Find contours
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(filled.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<cv::Point> approx;
    //cv::Mat dst = m_medianFrame.clone();

    for (int i = 0; i < contours.size(); i++)
    {
        // Approximate contour with accuracy proportional
        // to the contour perimeter
        cv::approxPolyDP(cv::Mat(contours[i]), approx, cv::arcLength(cv::Mat(contours[i]), true) * 0.02, true);

        // Skip small or non-convex objects
        if (std::fabs(cv::contourArea(contours[i])) < 300 || !cv::isContourConvex(approx))
            continue;

        else if (approx.size() >= 4 && approx.size() <= 6)
        {
            m_nozzleOutline = approx;
            // get bottom face
            std::vector<cv::Point> nozzleTip = {m_nozzleOutline[1], m_nozzleOutline[2]};
            m_originPoint = (nozzleTip[0] + nozzleTip[1]) / 2.0;
            //qDebug() << "origin point " << m_originPoint.x << m_originPoint.y;
            return;
        }
    }
    emit print_to_output_window("Could not detect nozzle, no valid contours found");
}

void DropletAnalyzer::estimate_image_scale()
{
    if (m_medianFrame.empty()) calculate_median_frame();
    if (m_nozzleOutline.empty()) detect_nozzle();


    if (m_nozzleOutline.size() >= 4) // if detected shape is rectangle
    {
        // get bottom face
        std::vector<cv::Point> nozzleTip = {m_nozzleOutline[1], m_nozzleOutline[2]};
        // get distance between last two points
        const double nozzleDiameter_px = cv::norm(nozzleTip[0]-nozzleTip[1]);
        const double newScale = m_nozzleTipDiameter_um / nozzleDiameter_px;
        m_cameraSettings.imagePixelSize_um = newScale;
        emit image_scale_estimated();
    }
    else
    {
        emit image_scale_estimation_failed();
    }
}

double DropletAnalyzer::get_nozzle_diameter()
{
    QMutexLocker lock(&m_mutex);
    return m_nozzleTipDiameter_um;
}

void DropletAnalyzer::set_nozzle_diameter(double diameter)
{
    QMutexLocker lock(&m_mutex);
    m_nozzleTipDiameter_um = diameter;
}

double DropletAnalyzer::get_strobe_step_time()
{
    QMutexLocker lock(&m_mutex);
    return m_strobeStepTime_us;
}

void DropletAnalyzer::set_strobe_step_time(double stepTime)
{
    QMutexLocker lock(&m_mutex);
    m_strobeStepTime_us = stepTime;
}

DropTrackingData DropletAnalyzer::get_droplet_tracking_data()
{
    QMutexLocker lock(&m_mutex);
    return m_trackingData;
}

#include "moc_dropletanalyzer.cpp"
