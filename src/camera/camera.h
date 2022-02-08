#ifndef CAMERA_H
#define CAMERA_H

#include "eventthread.h"
#include "property_class.h"
#include "ueye.h"
#include "utils.h"
#include <type_traits>
#include <QImage>
#include <QRgb>
#include <QSharedPointer>
#include <QString>
#include <QVariant>
#include <memory>
#include <utility>
#include <vector>
#include <QDebug>

#define CONFIG_SIZE (sizeof(AES_CONFIGURATION) - sizeof(CHAR) + sizeof(AES_PEAK_CONFIGURATION))
#define RANGE_SIZE (sizeof(AES_CONFIGURATION) - sizeof(CHAR) + sizeof(AES_PEAK_CONFIGURATION_RANGE))

Q_DECLARE_METATYPE(AES_PEAK_CONFIGURATION);
Q_DECLARE_METATYPE(AES_PEAK_CONFIGURATION_RANGE);

/*! \brief image buffer properties structure */
struct sBufferProps
{
    int width;
    int height;
    int colorformat;
    int bitspp;
    QImage::Format imgformat;
    QRgb *pRgbTable;
    int tableentries;
    int imageformat;
};

/*! \brief camera feature properties structure */
struct sCameraProps
{
    bool bUsesImageFormats;
    int nImgFmtNormal;
    int nImgFmtDefaultNormal;
    int nImgFmtTrigger;
    int nImgFmtDefaultTrigger;
};

typedef struct S_UEYE_IMAGE
{
    char *pBuf;
    INT nImageID;
    INT nImageSeqNum;
    INT nBufferSize;
    sBufferProps buffer_property;
} UEYE_IMAGE, *PUEYE_IMAGE;

class PeakAutoExposure : public Property
{
public:
    Property peakConfiguration;
    Property peakRange;
};

struct AutoExposure
{
    bool usePeakAutoImpl;
    PeakAutoExposure peak;
    Property mean;
};

struct SensorAutoFunctions
{
    PropertyBool useSensorAutoGain;
    PropertyBool useSensorAutoExposure;
    PropertyBool useSensorWhiteBalance;
    PropertyBool useSensorAutoFramerate;
};

namespace helper {
class LockUnlockSeqBuffer
{
public:
    LockUnlockSeqBuffer(HIDS hCam, const UEYE_IMAGE& image);
    ~LockUnlockSeqBuffer();

    NO_DISCARD bool OwnsLock() const   { return m_bOwnsLock; }
    void Unlock();

    LockUnlockSeqBuffer() = delete;
    LockUnlockSeqBuffer(const LockUnlockSeqBuffer&) = delete;
    LockUnlockSeqBuffer& operator=(const LockUnlockSeqBuffer&) = delete;

    int id()
    {
        return m_nSeqNum;
    }

    char* data()
    {
        return m_pcMem;
    }

    const sBufferProps& buffer_props()
    {
        return m_buffer_props;
    }

    UEYEIMAGEINFO image_info()
    {
        UEYEIMAGEINFO imageInfo;
        is_GetImageInfo(m_hCam, m_nSeqNum, &imageInfo, sizeof(imageInfo));

        return imageInfo;
    }

private:
    HIDS m_hCam;
    INT m_nSeqNum;
    char* m_pcMem;
    bool m_bOwnsLock;
    sBufferProps m_buffer_props;
};
}

typedef QSharedPointer<helper::LockUnlockSeqBuffer> ImageBufferPtr;

/*!
 * \brief Number of image buffer to alloc
 */
#define IMAGE_COUNT     5

NO_DISCARD static int GetNumberOfCameras()
{
    int count;
    is_GetNumberOfCameras(&count);
    return count;
}

NO_DISCARD static std::vector<UEYE_CAMERA_INFO> GetCameraList()
{
    std::vector<UEYE_CAMERA_INFO> cameras;
    auto camCount = GetNumberOfCameras();
    if (camCount > 0)
    {
        auto cameraList = std::unique_ptr<UEYE_CAMERA_LIST, decltype(std::free)*>{
            static_cast<UEYE_CAMERA_LIST*>(std::malloc(sizeof(DWORD) + camCount * sizeof(UEYE_CAMERA_INFO))), std::free };
        cameraList->dwCount = camCount;
        is_GetCameraList(cameraList.get());


        for (unsigned int i = 0; i < cameraList->dwCount; ++i)
        {
            cameras.push_back(cameraList->uci[i]);
        }
    }

    return cameras;
}

class Camera : public QObject
{
    Q_OBJECT
    EventThread *m_pEvDevice;

signals:
    void frameReceived(ImageBufferPtr buffer);
    void frameReceived();

    void captureStatusReceived();
    void deviceReconnected();
    void temperatureStatusChanged();

    void parameterChanged();
    void triggerModeChanged();

    void liveCaptureStateChanged(bool captureOn);

    void pixelclockListChanged(QVector<UINT> pixelClockList, UINT pixelClockIndex);

public slots:
    void eventreceived (unsigned int event);
    void resetErrorCounter();

public:
    struct sCameraProps m_CameraProps{};
    QRgb m_table[256]{};

    UEYE_IMAGE m_Images[IMAGE_COUNT] = {};

    uint64_t receivedFrames() const;
    uint64_t failedFrames() const;
    uint64_t reconnects() const;

    void resetToDefaults();

    Camera();
    ~Camera() override;

    void saveParameterSet() const;
    void saveParameterSet(const QString& file);
    void readParameterSet(const QString& file);
    void readParameterSet();

    void setUpdateFuncs();
    void initAutoFuncs();

    NO_DISCARD HIDS getCameraHandle() const {
        if(!m_hCamera)
        {
            return IS_INVALID_HIDS;
        }
        else
        {
            return m_hCamera;
        }
    }

    NO_DISCARD int Open(UEYE_CAMERA_INFO camera, bool allowUpload);
    void Close();
    NO_DISCARD bool isOpen() const;
    int SetupCapture();

    NO_DISCARD int searchDefImageFormats(int suppportMask) const;

    void captureVideo(bool triggered);
    void freezeVideo(bool triggered);
    void stopVideo();
    void forceTrigger() const;

    NO_DISCARD double framesPerSeconds() const;

    NO_DISCARD bool isLive() const;
    NO_DISCARD bool isFreerun() const;
    NO_DISCARD bool isTriggered() const;

    NO_DISCARD bool inUse() const;
    NO_DISCARD bool isXS() const;
    NO_DISCARD bool isXC() const;
    NO_DISCARD int getSID() const;
    NO_DISCARD bool isColor() const;
    NO_DISCARD bool isGlobalShutter() const;
    NO_DISCARD bool isRollingShutterMode() const;
    NO_DISCARD bool isUSB2() const;
    NO_DISCARD bool isETH() const;
    NO_DISCARD bool isPMC() const;
    NO_DISCARD bool isPEAK() const;

    void freeImages();
    void emptyImages();
    NO_DISCARD UEYE_IMAGE getImage(const char* pbuf);
    bool allocImages(const sBufferProps& buffer_property);

    NO_DISCARD bool hasMasterGain() const;
    NO_DISCARD bool hasRGain() const;
    NO_DISCARD bool hasGGain() const;
    NO_DISCARD bool hasBGain() const;
    NO_DISCARD bool hasAWB() const;
    NO_DISCARD HIDS handle() const;
    NO_DISCARD std::pair<int, int> GetMaxImageSize() const;
    NO_DISCARD QString SerNo() const;
    NO_DISCARD QString Model() const;
    NO_DISCARD QString WindowTitle() const;
    NO_DISCARD int getDeviceID() const;
    NO_DISCARD int getCameraID() const;

    void setTriggerMode(std::uint32_t mode);

    std::uint32_t triggerModeActual()
    {
        return m_triggerMode().toUInt();
    }

    std::uint32_t triggerModeSet()
    {
        return m_setTriggerMode;
    }

    void updateTriggerModeActual()
    {
        m_triggerMode.update();
    }

    std::uint32_t supportedTriggerModes()
    {
        return m_supportedTriggerModes;
    }

    void updateAll();
    void renumerate() const;

    NO_DISCARD bool isCameraMemoryEnabled() const;

    NO_DISCARD bool hasXsAES() const;
    NO_DISCARD bool hasXsAGES() const;

    NO_DISCARD UEYE_CAMERA_INFO getCamInfo() const;

    PropertyRange exposure;
    PropertyRange fps;
    PropertyRange pixelclock;

    PropertyBool bMaxExposure;
    PropertyBool bdualExposure;
    PropertyBool bLongExposure;

    PropertySize aoi;

    PropertyBool bColor;
    PropertyBool bJPEG;

    PropertyBool bHasAutoBlackLevel;
    PropertyBool bHasManualBlackLevel;
    PropertyBool bHasHardwareGamma;
    PropertyBool bHasSoftwareGamma;
    PropertyBool bHasGainBoost;

    PropertyBool bSoftwareGammaSet;
    PropertyBool bHardwareGammaSet;
    PropertyBool bGainBoostSet;

    PropertyRange MasterGain;
    PropertyRange softwareGamma;

    PropertyRange BlackLevel;

    PropertyRange RedGain;
    PropertyRange GreenGain;
    PropertyRange BlueGain;

    PropertyBool bAutoBlackLevel;

    // trigger delay
    PropertyRange TriggerDelay;
    PropertyBool bEnableTriggerDelaySet;
    PropertyRange triggerTimeout;
    PropertyRange triggerDebounceDelay;


    // flash delay
    PropertyRange FlashDelay;

    // flash duration
    PropertyRange FlashDuration;

    PropertyPwm pwm;

    Property edgeEnhancement{0};
    PropertyBool bHasEdgeEnhancement;

    PropertyBool bXsHasBacklightComp;
    PropertyBool bXsHasAES;
    PropertyBool bXsHasAGS;
    PropertyBool bXsHasAGES;

    PropertyRange xsJPEGCompression;
    PropertyRange xsSharpness;
    PropertyRange xsSaturation;
    Property xsLSCDefaultValue{0};

    PropertyRange manualFocus;
    Property autoFocus;
    PropertyBool focusOnceActive;
    PropertyBool hasFocus;

    UINT m_nNumberOfSupportedPixelClocks{};
    QVector<UINT> m_pixelClockList;
    UINT m_nPixelclockListIndex = 0;
    void updatePixelClockList();

    Property vFactor;
    Property hFactor;

    PropertyBool bAntiAliasingSupported;
    PropertyBool scalerActive;
    PropertyBool scalerAntiAliasingActive;

    PropertyRange burstSize;
    PropertyRange linePrescaler;
    PropertyRange framePrescaler;

    Property colorMode;
    Property colorConverter;

    Property binning;
    Property supportedBinning;
    Property subsampling;
    Property supportedSubsampling;

    Property sensorScaler;
    Property sensorInfo;

    PropertyBool autoGain;
    PropertyBool autoWhiteBalance;
    PropertyBool autoFramerate;
    AutoExposure autoExposure;
    SensorAutoFunctions useSensorAutoFunctions;
    PropertyBool runAutoExposureAndAutoGainOnce;

    bool markHotpixel = false;

    NO_DISCARD double GetAverageBandwidth();
    NO_DISCARD double GetCameraPeakBandwidth();
    NO_DISCARD double GetApiBandWidth() const;
    NO_DISCARD bool checkMemoryMode() const;
    INT loadImage(const QString& filePath);
    bool isGEV() const;
    bool isU3V() const;
    bool isSE() const;
    bool isUSB3() const;

private:
    Property m_triggerMode{IS_SET_TRIGGER_OFF};
    uint32_t m_setTriggerMode = {IS_SET_TRIGGER_OFF};

    std::uint32_t m_supportedTriggerModes;

    UEYE_CAMERA_INFO m_camInfo{};
    HIDS m_hCamera = 0;
    SENSORINFO m_SensorInfo{};
    UEYE_AUTO_INFO m_AutoInfo{};
    BOARDINFO m_boardInfo{};
    AES_PEAK_CONFIGURATION m_peakConfig{};

    bool m_memoryModeEn = false;

    void processCurrentImageInMemory();
};

Q_DECLARE_METATYPE(SENSORSCALERINFO)
Q_DECLARE_METATYPE(SENSORINFO)

namespace helper
{
    class AcquisitionGuard
    {
        /*
         * We cannot use properties here as their internal values are set before the setter functions.
         * If we reverse this the destructor won't work. Maybe call the set value stuff inside the setter functions.
         */
    public:
        explicit AcquisitionGuard(Camera *cam) : m_cam(cam)
        {
            is_AOI(cam->handle(), IS_AOI_IMAGE_GET_SIZE, &m_aoiSize, sizeof(m_aoiSize));
            m_cm = is_SetColorMode(cam->handle(), IS_GET_COLOR_MODE);
            m_wasLive = cam->isLive();
            m_triggered = cam->isTriggered();

            if (m_wasLive)
            {
                is_StopLiveVideo(cam->handle(), IS_FORCE_VIDEO_STOP);
            }
        }

        ~AcquisitionGuard()
        {
            IS_SIZE_2D newSize;
            is_AOI(m_cam->handle(), IS_AOI_IMAGE_GET_SIZE, &newSize, sizeof(newSize));
            auto new_cm = is_SetColorMode(m_cam->handle(), IS_GET_COLOR_MODE);

            if ((newSize.s32Width != m_aoiSize.s32Width) || (newSize.s32Height != m_aoiSize.s32Height) || new_cm != m_cm)
            {
                m_cam->SetupCapture();
            }

            if (m_wasLive)
            {
                m_cam->captureVideo(m_triggered);
            }
        }

        AcquisitionGuard(const AcquisitionGuard&) = delete;
        AcquisitionGuard& operator=(const AcquisitionGuard&) = delete;

        void setTriggered(bool triggered)
        {
            m_triggered = triggered;
        }
    private:
        Camera *m_cam;
        bool m_wasLive;
        bool m_triggered;
        IS_SIZE_2D m_aoiSize{};
        int m_cm;
    };
}

#endif // CAMERA_H
