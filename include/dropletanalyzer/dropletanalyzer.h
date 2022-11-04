#ifndef DROPLETANALYZER_H
#define DROPLETANALYZER_H

#include <QObject>
#include <QBasicTimer>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <optional>

#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "jetdrive.h"

typedef std::vector<std::vector<cv::Point>> Points2D;

struct DropletCameraSettings
{
    double imagePixelSize_um;
    double cameraPixelSize_um;
    int strobeStepTime_us; // I need to get this from the printer UI when possible..
    double image_pixel_size_from_mag(double magnification) const {return cameraPixelSize_um / magnification;}
    double calculate_lens_magnification() const {return cameraPixelSize_um / imagePixelSize_um;}
};

class DropletSlice
{
public:

    int y{};
    int leftPixel{};
    int rightPixel{};

    int droplet_width() const {return rightPixel - leftPixel;}
};

struct DropViewSettings
{
    bool showProcessedFrame = false;
    bool showDropletContour = false;
    bool showTracking = false;
    bool showNozzleOutline = false;
};

struct DropTrackingData
{
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> t;
    void clear() {x.clear();y.clear();t.clear();}
    double velocity_m_s {0.0};
    double intercept {0.0};
    double drop_angle_rad {0.0};
};

class DropletAnalyzer : public QObject
{
   Q_OBJECT
public:
   explicit DropletAnalyzer();
   ~DropletAnalyzer();
   QImage image() const;
   Q_PROPERTY(QImage image READ image NOTIFY image_ready USER true)
   //Q_PROPERTY(bool processAll READ processAll WRITE setProcessAll)
signals:
   void image_ready(const QImage &);
   void video_loaded();
   void video_load_failed();
   void video_analysis_successful();
   void image_scale_estimated();
   void image_scale_estimation_failed();
   void print_to_output_window(QString s);

public slots:
   DropletCameraSettings& camera_settings();
   void load_video(const std::string& filename);
   int get_number_of_frames();
   void show_frame(int frameNum);
   void update_view_settings(const DropViewSettings& viewSettings);
   void reset();
   void calculate_median_frame();
   double get_nozzle_diameter();
   void set_nozzle_diameter(double diameter);
   double get_strobe_step_time();
   void set_strobe_step_time(double stepTime);
   DropTrackingData get_droplet_tracking_data();
   void detect_nozzle();
   void estimate_image_scale();
   void analyze_video();
   void detect_contours();
   void track_droplet();
   void calculate_scaled_drop_pos();
   void generate_tracking_csv();
   void set_jetting_settings(const MicroJet& jetSettings);

private slots:
   void cleanup();
   bool is_video_loaded();
   cv::Point calculate_default_droplet_origin();
   void filter_data_by_residuals(double threshold);
   void rotate_data_by_angle(double angle_rad);
   void calculate_droplet_velocity();

private:
   QMutex m_mutex;

   int m_frameNum {0};
   std::vector<cv::Mat> m_video;
   int m_numFrames {0};
   cv::Mat m_medianFrame;
   double m_nozzleTipDiameter_um {0.0};
   double m_strobeStepTime_us {0.0};
   const cv::Point m_noTrackPoint = cv::Point(-100, 100);
   cv::Point m_originPoint = cv::Point(0,0);

   std::vector<cv::Point> m_nozzleOutline;
   std::vector<Points2D> m_dropletContours;
   std::vector<cv::Point> m_trackerPoints;

   DropTrackingData m_trackingData;

   QImage m_image;

   DropletCameraSettings m_cameraSettings;
   DropViewSettings m_viewSettings;

   std::optional<MicroJet> m_jetSettings;

   std::unique_ptr<QThread> m_thread;
};

#endif // DROPLETANALYZER_H
