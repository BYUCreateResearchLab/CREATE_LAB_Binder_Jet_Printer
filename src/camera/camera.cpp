#include "camera.h"

#include "queyeimage.h"
#include "utils.h"
#include <QDebug>
#include <QString>
#include <thread>
#include <utility>

#define IS_INTERFACE_MASK (IS_INTERFACE_TYPE_USB | IS_INTERFACE_TYPE_USB3 | IS_INTERFACE_TYPE_ETH | IS_INTERFACE_TYPE_PMC | IS_INTERFACE_TYPE_IDS_PEAK)

namespace helper
{
    LockUnlockSeqBuffer::LockUnlockSeqBuffer(HIDS hCam, const UEYE_IMAGE& image)
            : m_hCam(hCam), m_nSeqNum(image.nImageSeqNum), m_pcMem(image.pBuf), m_bOwnsLock(false), m_buffer_props(image.buffer_property)
    {
        qRegisterMetaType<ImageBufferPtr>("ImageBufferPtr");

        INT nRet = is_LockSeqBuf(m_hCam, m_nSeqNum, m_pcMem);

        m_bOwnsLock = (IS_SUCCESS == nRet);
    }

    LockUnlockSeqBuffer::~LockUnlockSeqBuffer()
    {
        Unlock();
    }

    void LockUnlockSeqBuffer::Unlock()
    {
        if (m_bOwnsLock)
        {
            is_UnlockSeqBuf(m_hCam, m_nSeqNum, m_pcMem);
            m_bOwnsLock = false;
        }
    }
}

Camera::Camera()
{
    for (int i = 0; i < 256; i++)
        m_table[i] = qRgb (i, i, i);

    setUpdateFuncs();
    initAutoFuncs();

    m_pEvDevice = new EventThread(this);
    connect(m_pEvDevice, &EventThread::eventhappen, this, &Camera::eventreceived, Qt::DirectConnection);
    connect(this, SELECT<>::OVERLOAD_OF(&Camera::frameReceived), this, &Camera::processCurrentImageInMemory, Qt::DirectConnection);

    connect(&m_triggerMode, &Property::valueChanged, this, &Camera::triggerModeChanged);
}

void Camera::captureVideo(bool triggered)
{
    if(triggered)
    {
        m_triggerMode.setValue(m_setTriggerMode);
    }
    else
    {
        m_triggerMode.setValue(IS_TRIGGER_SOURCE_OFF);
    }

    if (m_CameraProps.bUsesImageFormats)
    {
        int format = triggered && m_CameraProps.nImgFmtTrigger != 0 ? m_CameraProps.nImgFmtTrigger : m_CameraProps.nImgFmtNormal;
        auto ret = is_ImageFormat(handle(),
            IMGFRMT_CMD_SET_FORMAT,
            reinterpret_cast<void*>(&format),
            sizeof(format));
        if (ret != IS_SUCCESS)
        {
            qDebug("Error applying special image format %d", ret);
        }
        else
        {
            SetupCapture();
        }
    }

    int nRet = is_CaptureVideo(m_hCamera, IS_DONT_WAIT);
    if (nRet == IS_SUCCESS)
    {
        emit liveCaptureStateChanged(true);
    }
    else
    {
        qDebug() << "Error: " << nRet;
    }
}

void Camera::freezeVideo(bool triggered)
{
    if(triggered)
    {
        m_triggerMode.setValue(m_setTriggerMode);
    }
    else
    {
        m_triggerMode.setValue(IS_TRIGGER_SOURCE_OFF);
    }

    if (m_CameraProps.bUsesImageFormats)
    {
        int format = triggered && m_CameraProps.nImgFmtTrigger != 0 ? m_CameraProps.nImgFmtTrigger : m_CameraProps.nImgFmtNormal;
        auto ret = is_ImageFormat(handle(),
            IMGFRMT_CMD_SET_FORMAT,
            reinterpret_cast<void*>(&format),
            sizeof(format));
        if (ret != IS_SUCCESS)
        {
            qDebug("Error applying special image format %d", ret);
        }
        else
        {
            SetupCapture();
        }
    }

    int nRet = is_FreezeVideo(m_hCamera, isXC() ? IS_WAIT : IS_DONT_WAIT);
    if (nRet != IS_SUCCESS)
    {
        qDebug() << "Error: " << nRet;
    }
}

void Camera::stopVideo()
{
    int nRet = is_StopLiveVideo(m_hCamera, IS_FORCE_VIDEO_STOP);
    if (nRet == IS_SUCCESS)
    {
        emit liveCaptureStateChanged(false);
    }
    else
    {
        qDebug() << "Error: " << nRet;
    }
}

void Camera::renumerate() const
{
    is_Renumerate(m_hCamera, IS_RENUM_BY_CAMERA);
}

bool Camera::isFreerun() const
{
    return is_SetExternalTrigger(m_hCamera, IS_GET_EXTERNALTRIGGER) == IS_SET_TRIGGER_OFF;
}

void Camera::eventreceived (unsigned int event)
{
    switch (event)
    {

    case IS_SET_EVENT_CAPTURE_STATUS:
        captureStatusReceived();
        break;
    case IS_SET_EVENT_PMC_IMAGE_PARAMS_CHANGED:
        //TODO:reOpenCamera();
        break;
    case IS_SET_EVENT_FRAME:
        emit frameReceived();
        break;
    case IS_SET_EVENT_DEVICE_RECONNECTED:
        deviceReconnected();
        break;
    case IS_SET_EVENT_TEMPERATURE_STATUS:
        temperatureStatusChanged();
        break;

    default:
        break;
    }
}

uint64_t Camera::receivedFrames() const
{
     return m_pEvDevice->getCntFromEvent(IS_SET_EVENT_FRAME);
}

uint64_t Camera::failedFrames() const
{
    return m_pEvDevice->getCntFromEvent(IS_SET_EVENT_CAPTURE_STATUS);
}

uint64_t Camera::reconnects() const
{
    return m_pEvDevice->getCntFromEvent(IS_SET_EVENT_DEVICE_RECONNECTED);
}

void Camera::setUpdateFuncs()
{
    exposure.setFuncGet([this]() -> QVariant {
                            double exp = 0;
                            if(is_Exposure(this->m_hCamera, IS_EXPOSURE_CMD_GET_EXPOSURE, &exp, sizeof(exp)) != IS_SUCCESS)
                            {
                                return {};
                            }

                            return exp;
                        });

    exposure.setFuncSet([this](const QVariant& m) -> bool {
        double exp = m.toDouble();
        if(is_Exposure(this->m_hCamera, IS_EXPOSURE_CMD_SET_EXPOSURE, &exp, sizeof(exp)) != IS_SUCCESS)
        {
            return false;
        }

        return true;
    });

    exposure.setRangeFunc([this](QVariant &min, QVariant &max, QVariant &inc) -> bool {
        double exp[3];

        if(is_Exposure(this->m_hCamera, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE, &exp, sizeof(exp)) != IS_SUCCESS)
        {
            return false;
        }

        min = exp[0];
        max = exp[1];
        inc = exp[2];

        return true;
    });

    bLongExposure.setFuncGet([this]() -> QVariant {
      UINT longExp = 0;
      if(is_Exposure(this->m_hCamera, IS_EXPOSURE_CMD_GET_LONG_EXPOSURE_ENABLE, &longExp, sizeof(longExp)) != IS_SUCCESS)
      {
          return {};
      }

      return longExp != 0;
    });

    bLongExposure.setFuncSet([this](const QVariant& m) -> bool {
      UINT longExp = m.toUInt();
      if(is_Exposure(this->m_hCamera, IS_EXPOSURE_CMD_SET_LONG_EXPOSURE_ENABLE, &longExp, sizeof(longExp)) != IS_SUCCESS)
      {
          return false;
      }

      return true;
    });

    fps.setFuncGet([this]() -> QVariant {
                       double myfps = 0;
                       if(is_SetFrameRate(this->m_hCamera, IS_GET_FRAMERATE, &myfps) != IS_SUCCESS)
                       {
                           return {};
                       }

                       return myfps;
                   });

    fps.setFuncSet([this](const QVariant& v) -> bool {
        double myfps = v.toDouble();
        double new_fps;
        return is_SetFrameRate(this->m_hCamera, myfps, &new_fps) == IS_SUCCESS;
    });

    fps.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        double lmin, lmax, linc;
        if(is_GetFrameTimeRange(this->m_hCamera, &lmin, &lmax, &linc) == IS_SUCCESS)
        {
            max = 1.0 / lmin;
            min = 1.0 / lmax;

            if (this->getSID() == IS_SENSOR_UI1013XC )
            {
                inc = 0.5; // increment should be 0.5;
            }
            else
            {
                inc = linc + 0.05; // increment should be min 0.05
            }


            return true;
        }
        else
        {
            return false;
        }
    });

    fps.validateInc(false);

    pixelclock.setFuncGet([this]() -> QVariant {
                              INT pxclock = 0;
                              if(is_PixelClock(this->m_hCamera, IS_PIXELCLOCK_CMD_GET, &pxclock, sizeof(pxclock)) != IS_SUCCESS)
                              {
                                  return {};
                              }

                              return pxclock;
                          });

    pixelclock.setFuncSet([this](const QVariant& v) -> bool {
        UINT pxclock = v.toUInt();
        return is_PixelClock(this->m_hCamera, IS_PIXELCLOCK_CMD_SET, &pxclock, sizeof(pxclock)) == IS_SUCCESS;
    });

    pixelclock.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        UINT nRange[3];
        if(is_PixelClock(this->m_hCamera, IS_PIXELCLOCK_CMD_GET_RANGE, &nRange, sizeof(nRange)) != IS_SUCCESS)
        {
            return false;
        }
        min = nRange[0];
        max = nRange[1];
        inc = nRange[2];

        return true;
    });

    aoi.setFuncGet([this]() -> QRect {
                       IS_RECT rect;
                       if(is_AOI(this->m_hCamera, IS_AOI_IMAGE_GET_AOI, &rect, sizeof(rect)) != IS_SUCCESS)
                       {
                           qDebug() << "Error: is_AOI(IS_AOI_IMAGE_GET_AOI)";
                           return {};
                       }
                       return {rect.s32X, rect.s32Y, rect.s32Width, rect.s32Height};
                   });

    aoi.setFuncSet([this](QRect r) -> bool {
        helper::AcquisitionGuard guard(this);

        UINT nAbsPosX = 0;
        UINT nAbsPosY = 0;

        int ret = is_AOI(getCameraHandle(), IS_AOI_IMAGE_GET_POS_X_ABS, reinterpret_cast<void*>(&nAbsPosX) , sizeof(nAbsPosX));

        if (ret == IS_SUCCESS)
        {
            ret = is_AOI(getCameraHandle(), IS_AOI_IMAGE_GET_POS_Y_ABS, reinterpret_cast<void*>(&nAbsPosY),
                    sizeof(nAbsPosY));
        }

        IS_RECT rect;
        rect.s32X = r.x();
        rect.s32Y = r.y();
        rect.s32Width = r.width();
        rect.s32Height = r.height();

        if (ret == IS_SUCCESS)
        {
            rect.s32X |= static_cast<decltype(rect.s32X)>(nAbsPosX);
            rect.s32Y |= static_cast<decltype(rect.s32Y)>(nAbsPosY);
        }

        bool OK = is_AOI(this->m_hCamera, IS_AOI_IMAGE_SET_AOI, &rect, sizeof(rect)) == IS_SUCCESS;

        return OK;
    });

    aoi.setSizeRangeFunc([this](QSize &min, QSize &max, QSize &inc) -> bool {
        IS_SIZE_2D sizeTemp;
        if(is_AOI(this->m_hCamera, IS_AOI_IMAGE_GET_SIZE_MAX, reinterpret_cast<void*>(&sizeTemp), sizeof(sizeTemp)) != IS_SUCCESS)
        {
            return false;
        }
        max = {sizeTemp.s32Width, sizeTemp.s32Height};

        if(is_AOI(this->m_hCamera, IS_AOI_IMAGE_GET_SIZE_MIN, reinterpret_cast<void*>(&sizeTemp), sizeof(sizeTemp)) != IS_SUCCESS)
        {
            return false;
        }
        min = {sizeTemp.s32Width, sizeTemp.s32Height};

        if(is_AOI(this->m_hCamera, IS_AOI_IMAGE_GET_SIZE_INC, reinterpret_cast<void*>(&sizeTemp), sizeof(sizeTemp)) != IS_SUCCESS)
        {
            return false;
        }
        inc = {sizeTemp.s32Width, sizeTemp.s32Height};

        return true;
    });

    aoi.setSizePosFunc([this](QPoint &min, QPoint &max, QPoint &inc) -> bool {
        IS_POINT_2D pointTemp;
        if(is_AOI(this->m_hCamera, IS_AOI_IMAGE_GET_POS_MAX, reinterpret_cast<void*>(&pointTemp), sizeof(pointTemp)) != IS_SUCCESS)
        {
            return false;
        }
        max = {pointTemp.s32X,pointTemp.s32Y};

        if(is_AOI(this->m_hCamera, IS_AOI_IMAGE_GET_POS_MIN, reinterpret_cast<void*>(&pointTemp), sizeof(pointTemp)) != IS_SUCCESS)
        {
            return false;
        }
        min = {pointTemp.s32X,pointTemp.s32Y};

        if(is_AOI(this->m_hCamera, IS_AOI_IMAGE_GET_POS_INC, reinterpret_cast<void*>(&pointTemp), sizeof(pointTemp)) != IS_SUCCESS)
        {
            return false;
        }
        inc = {pointTemp.s32X,pointTemp.s32Y};

        return true;
    });


    MasterGain.setFuncGet([this]() -> QVariant {
                              int gain = is_SetHardwareGain(this->m_hCamera, IS_GET_MASTER_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
                              return gain;
                          });

    MasterGain.setFuncSet([this](const QVariant& v) -> bool {
        return is_SetHardwareGain(this->m_hCamera, v.toInt(), IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) == IS_SUCCESS;
    });

    MasterGain.setRangeFunc([](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        min = 0;
        max = 100;
        inc = 1;

        return true;
    });

    RedGain.setFuncGet([this]() -> QVariant {
                           return is_SetHardwareGain(this->m_hCamera, IS_GET_RED_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
                       });

    RedGain.setFuncSet([this](const QVariant& v) -> bool {
        return is_SetHardwareGain(this->m_hCamera, IS_IGNORE_PARAMETER, v.toInt(), IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER) == IS_SUCCESS;
    });

    RedGain.setRangeFunc([](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        min = 0;
        max = 100;
        inc = 1;

        return true;
    });

    BlueGain.setFuncGet([this]() -> QVariant {
                            return is_SetHardwareGain(this->m_hCamera, IS_GET_BLUE_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
                        });

    BlueGain.setFuncSet([this](const QVariant& v) -> bool {
        return is_SetHardwareGain(this->m_hCamera, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, v.toInt()) == IS_SUCCESS;
    });

    BlueGain.setRangeFunc([](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        min = 0;
        max = 100;
        inc = 1;

        return true;
    });

    GreenGain.setFuncGet([this]() -> QVariant {
                             return is_SetHardwareGain(this->m_hCamera, IS_GET_GREEN_GAIN, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
                         });

    GreenGain.setFuncSet([this](const QVariant& v) -> bool {
        return is_SetHardwareGain(this->m_hCamera, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, v.toInt(), IS_IGNORE_PARAMETER) == IS_SUCCESS;
    });

    GreenGain.setRangeFunc([](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        min = 0;
        max = 100;
        inc = 1;

        return true;
    });

    softwareGamma.setFuncGet([this]() -> QVariant {
                                 INT gamma = 0;

                                 if(is_Gamma(this->m_hCamera, IS_GAMMA_CMD_GET, &gamma, sizeof(gamma)) == IS_SUCCESS)
                                 {
                                     return gamma;
                                 }
                                 else
                                 {
                                     return {};
                                 }
                             });

    softwareGamma.setRangeFunc([](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        min = IS_GAMMA_VALUE_MIN;
        max = IS_GAMMA_VALUE_MAX;
        inc = 1;

        return true;
    });

    softwareGamma.setFuncSet([this](const QVariant& v) -> bool {
        INT gamma = v.toInt();
        auto nret = is_Gamma(this->m_hCamera, IS_GAMMA_CMD_SET, &gamma, sizeof(gamma));
        return nret == IS_SUCCESS;
    });

    BlackLevel.setFuncGet([this]() -> QVariant {
                              INT blacklevel = 0;

                              if(is_Blacklevel(this->m_hCamera, IS_BLACKLEVEL_CMD_GET_OFFSET, &blacklevel, sizeof(blacklevel)) == IS_SUCCESS)
                              {
                                  return blacklevel;
                              }
                              else
                              {
                                  return {};
                              }
                          });

    BlackLevel.setFuncSet([this](const QVariant& v) -> bool {
        INT blacklevel = v.toInt();
        return is_Blacklevel(this->m_hCamera, IS_BLACKLEVEL_CMD_SET_OFFSET, &blacklevel, sizeof(blacklevel)) == IS_SUCCESS;
    });

    BlackLevel.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        IS_RANGE_S32 bl_range;
        if(is_Blacklevel(this->m_hCamera, IS_BLACKLEVEL_CMD_GET_OFFSET_RANGE, &bl_range, sizeof(bl_range)) == IS_SUCCESS)
        {
            min = bl_range.s32Min;
            max = bl_range.s32Max;
            inc = bl_range.s32Inc;
            return true;
        }
        else
        {
            return false;
        }
    });

    TriggerDelay.setFuncGet([this]() -> QVariant {
                                return is_SetTriggerDelay(this->m_hCamera, IS_GET_TRIGGER_DELAY);
                            });

    TriggerDelay.setFuncSet([this](const QVariant& v) -> bool {
        INT delay = v.toInt();
        return is_SetTriggerDelay(this->m_hCamera, delay) == IS_SUCCESS;
    });

    TriggerDelay.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        min = is_SetTriggerDelay(this->m_hCamera, IS_GET_MIN_TRIGGER_DELAY);
        max = is_SetTriggerDelay(this->m_hCamera, IS_GET_MAX_TRIGGER_DELAY);
        inc = is_SetTriggerDelay(this->m_hCamera, IS_GET_TRIGGER_DELAY_GRANULARITY);

        return true;
    });

    //check merge delay,dur
    FlashDelay.setFuncGet([this]() -> QVariant {
                              IO_FLASH_PARAMS flashParams;

                              if(is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS, &flashParams, sizeof(flashParams)) == IS_SUCCESS)
                              {
                                  return flashParams.s32Delay;
                              }
                              else
                              {
                                  return {};
                              }
                          });

    FlashDelay.setFuncSet([this](const QVariant& v) -> bool {
        IO_FLASH_PARAMS flashParams;

        if(is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS, &flashParams, sizeof(flashParams)) == IS_SUCCESS)
        {
            flashParams.s32Delay = v.toInt();
            if(is_IO(this->m_hCamera, IS_IO_CMD_FLASH_SET_PARAMS, &flashParams, sizeof(flashParams)) == IS_SUCCESS)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    });

    FlashDelay.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        IO_FLASH_PARAMS lmin, lmax, linc;
        bool ret1, ret2, ret3;
        ret1 = is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS_MIN, &lmin, sizeof(lmin)) == IS_SUCCESS;
        ret2 = is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS_MAX, &lmax, sizeof(lmax)) == IS_SUCCESS;
        ret3 = is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS_INC, &linc, sizeof(linc)) == IS_SUCCESS;

        if(ret1 && ret2 && ret3)
        {
            min = lmin.s32Delay;
            max = lmax.s32Delay;
            inc = linc.s32Delay;

            return true;
        }
        else
        {
            return false;
        }
    });

    FlashDuration.setFuncGet([this]() -> QVariant {
                                 IO_FLASH_PARAMS flashParams;

                                 if(is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS, &flashParams, sizeof(flashParams)) == IS_SUCCESS)
                                 {
                                     return flashParams.u32Duration;
                                 }
                                 else
                                 {
                                     return {};
                                 }
                             });

    FlashDuration.setFuncSet([this](const QVariant& v) -> bool {
        IO_FLASH_PARAMS flashParams;

        if(is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS, &flashParams, sizeof(flashParams)) == IS_SUCCESS)
        {
            flashParams.u32Duration = v.toUInt();
            if(is_IO(this->m_hCamera, IS_IO_CMD_FLASH_SET_PARAMS, &flashParams, sizeof(flashParams)) == IS_SUCCESS)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    });

    FlashDuration.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        IO_FLASH_PARAMS lmin, lmax, linc;
        bool ret1, ret2, ret3;
        ret1 = is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS_MIN, &lmin, sizeof(lmin)) == IS_SUCCESS;
        ret2 = is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS_MAX, &lmax, sizeof(lmax)) == IS_SUCCESS;
        ret3 = is_IO(this->m_hCamera, IS_IO_CMD_FLASH_GET_PARAMS_INC, &linc, sizeof(linc)) == IS_SUCCESS;

        if(ret1 && ret2 && ret3)
        {
            min = lmin.u32Duration;
            max = lmax.u32Duration;
            inc = linc.u32Duration;

            return true;
        }
        else
        {
            return false;
        }
    });

    FlashDuration.addRangeException(0);

    edgeEnhancement.setFuncGet([this](){
        int value = IS_EDGE_EN_DISABLE;
        int nRet = is_EdgeEnhancement(m_hCamera, IS_EDGE_ENHANCEMENT_CMD_GET, &value, sizeof(value));
        if (nRet == IS_SUCCESS)
        {
            return value;
        }
        return IS_EDGE_EN_DISABLE;
    });

    bHasEdgeEnhancement.setFuncGet([this]() {
        int value = IS_EDGE_EN_DISABLE;
        int nRet = is_EdgeEnhancement(m_hCamera, IS_EDGE_ENHANCEMENT_CMD_GET, &value, sizeof(value));
        return nRet == IS_SUCCESS;
    });

    vFactor.setFuncGet([](){
        return false;
    });

    hFactor.setFuncGet([](){
        return false;
    });

    bAutoBlackLevel.setFuncGet([this]() {
        int nMode = IS_AUTO_BLACKLEVEL_OFF;
        auto nRet =
            is_Blacklevel(m_hCamera, IS_BLACKLEVEL_CMD_GET_MODE, reinterpret_cast<void*>(&nMode), sizeof(nMode));
        return nRet == IS_SUCCESS && nMode == IS_AUTO_BLACKLEVEL_ON;
    });

    bEnableTriggerDelaySet.setFuncGet([this]() {
        TriggerDelay.update();
        return TriggerDelay().toInt() > TriggerDelay.range()[0].toInt();
    });

    bMaxExposure.setFuncGet([](){
        return false;
    });

    bdualExposure.setFuncGet([](){
        return false;
    });

    bColor.setFuncGet([this]() {
        SENSORINFO sensInfo;

        if (is_GetSensorInfo(m_hCamera, &sensInfo) == IS_SUCCESS)
        {
            if (IS_COLORMODE_BAYER == sensInfo.nColorMode)
            {
                return true;
            }
        }
        return false;
    });

    bJPEG.setFuncGet([this]() {
        int currentMode = IS_CONV_MODE_NONE;
        int nRet = is_GetColorConverter(m_hCamera, colorMode().toInt(), &currentMode, nullptr, nullptr);
        return nRet == IS_SUCCESS && currentMode == IS_CONV_MODE_JPEG;
    });

    bJPEG.setFuncSet([this](const QVariant& enable) -> int {
        helper::AcquisitionGuard guard(this);

        int nRet = IS_SUCCESS;
        nRet = is_SetColorConverter(m_hCamera, colorMode().toInt(), enable.toBool() ? IS_CONV_MODE_JPEG : IS_CONV_MODE_SOFTWARE);
        if (nRet != IS_SUCCESS){
            qDebug("Error setting jpeg mode to %d: %i", enable.toBool(), nRet);
        }
        return nRet;
    });

    bHasAutoBlackLevel.setFuncGet([this]() {
        INT nBlacklevelCaps;
        auto nRet = is_Blacklevel(m_hCamera, IS_BLACKLEVEL_CMD_GET_CAPS, &nBlacklevelCaps, sizeof(nBlacklevelCaps));
        return nRet == IS_SUCCESS && (nBlacklevelCaps & IS_BLACKLEVEL_CAP_SET_AUTO_BLACKLEVEL) != 0;
    });

    bHasManualBlackLevel.setFuncGet([this]() {
        INT nBlacklevelCaps;
        auto nRet = is_Blacklevel(m_hCamera, IS_BLACKLEVEL_CMD_GET_CAPS, &nBlacklevelCaps, sizeof(nBlacklevelCaps));
        return nRet == IS_SUCCESS && (nBlacklevelCaps & IS_BLACKLEVEL_CAP_SET_OFFSET) != 0;
    });

    bHasHardwareGamma.setFuncGet([this](){
        return (is_SetHardwareGamma (m_hCamera, IS_GET_HW_SUPPORTED_GAMMA) & IS_SET_HW_GAMMA_ON);
    });

    bHasSoftwareGamma.setFuncGet([this]() {
        INT nGamma;
        if (getSID() != IS_SENSOR_UI1013XC)
        {
            int ret = is_Gamma(m_hCamera, IS_GAMMA_CMD_GET, &nGamma, sizeof(nGamma));
            return (ret == IS_SUCCESS);
        }
        else
        {
            // no gamma support fox UI-1013XC
            return false;
        }
    });

    bHasGainBoost.setFuncGet(
        [this]() { return (is_SetGainBoost(m_hCamera, IS_GET_SUPPORTED_GAINBOOST) & IS_SET_GAINBOOST_ON); });

    xsJPEGCompression.setFuncGet([this]() -> QVariant {
                                     INT comp = 0;
                                     if(is_DeviceFeature(this->m_hCamera, IS_DEVICE_FEATURE_CMD_GET_JPEG_COMPRESSION, reinterpret_cast<void*>(&comp), sizeof(comp)) != IS_SUCCESS)
                                     {
                                         return {};
                                     }
                                     return comp;

                                 });
    xsJPEGCompression.setFuncSet([this](const QVariant& v) -> bool {
        INT comp = v.toInt();
        return is_DeviceFeature(this->m_hCamera, IS_DEVICE_FEATURE_CMD_SET_JPEG_COMPRESSION, reinterpret_cast<void*>(&comp), sizeof(comp)) == IS_SUCCESS;
    });
    xsJPEGCompression.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        // get JPEG compression Range
        IS_RANGE_S32 range{};

        if(is_DeviceFeature(this->m_hCamera, IS_DEVICE_FEATURE_CMD_GET_JPEG_COMPRESSION_RANGE, reinterpret_cast<void*>(&range), sizeof(range)) != IS_SUCCESS)
        {
            return false;
        }
        min = range.s32Min;
        max = range.s32Max;
        inc = range.s32Inc;
        return true;
    });
    xsSharpness.setFuncGet([this]() -> QVariant {
                               INT tmp = 0;

                               if(is_Sharpness(this->m_hCamera, SHARPNESS_CMD_GET_VALUE, &tmp, sizeof(tmp)) != IS_SUCCESS)
                               {
                                   return {};
                               }
                               return tmp;
                           });
    xsSharpness.setFuncSet([this](const QVariant& v) -> bool {
        INT tmp = v.toInt();

        return is_Sharpness(this->m_hCamera, SHARPNESS_CMD_SET_VALUE, &tmp, sizeof(tmp)) == IS_SUCCESS;
    });
    xsSharpness.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        INT tmp = 0;

        if(is_Sharpness(this->m_hCamera, SHARPNESS_CMD_GET_MIN_VALUE, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min = tmp;

        if(is_Sharpness(this->m_hCamera, SHARPNESS_CMD_GET_MAX_VALUE, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        max = tmp;

        if(is_Sharpness(this->m_hCamera, SHARPNESS_CMD_GET_INCREMENT, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        inc = tmp;

        return true;
    });

    xsSaturation.setFuncGet([this]() -> QVariant {
                                INT tmp = 0;

                                if(is_Saturation (this->m_hCamera, SATURATION_CMD_GET_VALUE, &tmp, sizeof(tmp)) != IS_SUCCESS)
                                {
                                    return {};
                                }
                                return tmp;
                            });
    xsSaturation.setFuncSet([this](const QVariant& v) -> bool {
        INT tmp = v.toInt();

        return is_Saturation(this->m_hCamera, SATURATION_CMD_SET_VALUE, &tmp, sizeof(tmp)) == IS_SUCCESS;
    });
    xsSaturation.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        INT tmp = 0;

        if(is_Saturation(this->m_hCamera, SATURATION_CMD_GET_MIN_VALUE, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min = tmp;

        if(is_Saturation(this->m_hCamera, SATURATION_CMD_GET_MAX_VALUE, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        max = tmp;

        if(is_Saturation(this->m_hCamera, SATURATION_CMD_GET_INCREMENT, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        inc = tmp;

        return true;
    });

    manualFocus.setFuncGet([this]() -> QVariant {
                               INT tmp = 0;

                               if(is_Focus (this->m_hCamera, FOC_CMD_GET_MANUAL_FOCUS, &tmp, sizeof(tmp)) != IS_SUCCESS)
                               {
                                   return {};
                               }
                               return tmp;
                           });
    manualFocus.setFuncSet([this](const QVariant& v) -> bool {
        INT tmp = v.toInt();

        return is_Focus(this->m_hCamera, FOC_CMD_SET_MANUAL_FOCUS, &tmp, sizeof(tmp)) == IS_SUCCESS;
    });
    manualFocus.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        INT tmp = 0;

        if(is_Focus(this->m_hCamera, FOC_CMD_GET_MANUAL_FOCUS_MIN, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min = tmp;

        if(is_Focus(this->m_hCamera, FOC_CMD_GET_MANUAL_FOCUS_MAX, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        max = tmp;

        if(is_Focus(this->m_hCamera, FOC_CMD_GET_MANUAL_FOCUS_INC, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        inc = tmp;

        return true;
    });

    pwm.setFuncGet([this](QVariantList& v) -> bool {
        IO_PWM_PARAMS tmp;
        v.clear();

        if(is_IO(this->m_hCamera, IS_IO_CMD_PWM_GET_PARAMS, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        v.push_back(tmp.dblFrequency_Hz);
        v.push_back(tmp.dblDutyCycle);

        return true;
    });

    pwm.setFuncSet([this](QVariantList& v) -> bool {
        if(v.size() != 2)
        {
            return false;
        }

        IO_PWM_PARAMS tmp{};
        tmp.dblFrequency_Hz = v[0].toDouble();
        tmp.dblDutyCycle = v[1].toDouble();

        auto ret = is_IO(this->m_hCamera, IS_IO_CMD_PWM_SET_PARAMS, &tmp, sizeof(tmp));

        return ret != IS_SUCCESS;
    });

    pwm.setFuncRange([this](QVariantList& min, QVariantList& max, QVariantList& inc) -> bool {
        IO_PWM_PARAMS tmp{};
        min.clear();
        max.clear();
        inc.clear();

        if(is_IO(this->m_hCamera, IS_IO_CMD_PWM_GET_PARAMS_MIN, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min.push_back(tmp.dblFrequency_Hz);
        min.push_back(tmp.dblDutyCycle);

        if(is_IO(this->m_hCamera, IS_IO_CMD_PWM_GET_PARAMS_MAX, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        max.push_back(tmp.dblFrequency_Hz);
        max.push_back(tmp.dblDutyCycle);

        if(is_IO(this->m_hCamera, IS_IO_CMD_PWM_GET_PARAMS_INC, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        inc.push_back(tmp.dblFrequency_Hz);
        inc.push_back(tmp.dblDutyCycle);

        return true;
    });

    m_triggerMode.setFuncGet([this]() -> QVariant{
                               return is_SetExternalTrigger(this->m_hCamera, IS_GET_EXTERNALTRIGGER);
                           });
    m_triggerMode.setFuncSet([this](const QVariant& v) -> bool{
                               return is_SetExternalTrigger(this->m_hCamera, v.toInt()) == IS_SUCCESS;
                           });


    triggerTimeout.setFuncGet([this]() -> QVariant {
                                  UINT tmp = 0;

                                  if(is_GetTimeout(this->m_hCamera, IS_TRIGGER_TIMEOUT, &tmp) != IS_SUCCESS)
                                  {
                                      return {};
                                  }
                                  return tmp;
                              });
    triggerTimeout.setFuncSet([this](const QVariant& v) -> bool {
        UINT tmp = v.toUInt();

        return is_SetTimeout(this->m_hCamera, IS_TRIGGER_TIMEOUT, tmp) == IS_SUCCESS;
    });
    triggerTimeout.setRangeFunc([](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        min = 0;
        max = 429496729;
        inc = 1;

        return true;
    });

    triggerDebounceDelay.setFuncGet([this]() -> QVariant {
                                        UINT delay = 0;
                                        if(is_TriggerDebounce(this->m_hCamera, TRIGGER_DEBOUNCE_CMD_GET_DELAY_TIME, &delay, sizeof(delay)) != IS_SUCCESS)
                                        {
                                            return {};
                                        }
                                        return delay;
                                    });
    triggerDebounceDelay.setFuncSet([this](const QVariant& v) -> bool {
        UINT tmp = v.toUInt();

        return is_TriggerDebounce(this->m_hCamera, TRIGGER_DEBOUNCE_CMD_SET_DELAY_TIME, &tmp, sizeof(tmp)) == IS_SUCCESS;
    });
    triggerDebounceDelay.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        UINT tmp = 0;
        if(is_TriggerDebounce(this->m_hCamera, TRIGGER_DEBOUNCE_CMD_GET_DELAY_TIME_MIN, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min = tmp;

        if(is_TriggerDebounce(this->m_hCamera, TRIGGER_DEBOUNCE_CMD_GET_DELAY_TIME_MAX, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        max = tmp;

        if(is_TriggerDebounce(this->m_hCamera, TRIGGER_DEBOUNCE_CMD_GET_DELAY_TIME_INC, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        inc = tmp;

        return true;
    });

    burstSize.setFuncGet([this]() -> QVariant {
                             UINT tmp = 0;
                             if(is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_GET_BURST_SIZE, &tmp, sizeof(tmp)) != IS_SUCCESS)
                             {
                                 return {};
                             }
                             return tmp;
                         });
    burstSize.setFuncSet([this](const QVariant& v) -> bool {
        UINT tmp = v.toUInt();
        helper::AcquisitionGuard guard(this);

        return is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_SET_BURST_SIZE, &tmp, sizeof(tmp)) == IS_SUCCESS;
    });
    burstSize.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        RANGE_OF_VALUES_U32 tmp{};
        if(is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_GET_BURST_SIZE_RANGE, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min = tmp.u32Minimum;
        max = tmp.u32Maximum;
        inc = tmp.u32Increment;

        return true;
    });


    linePrescaler.setFuncGet([this]() -> QVariant {
                                 UINT tmp = 0;
                                 if(is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_GET_LINE_PRESCALER, &tmp, sizeof(tmp)) != IS_SUCCESS)
                                 {
                                     return {};
                                 }
                                 return tmp;
                             });
    linePrescaler.setFuncSet([this](const QVariant& v) -> bool {
        UINT tmp = v.toUInt();
        helper::AcquisitionGuard guard(this);
        return is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_SET_LINE_PRESCALER, &tmp, sizeof(tmp)) == IS_SUCCESS;
    });
    linePrescaler.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        RANGE_OF_VALUES_U32 tmp{};
        if(is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_GET_LINE_PRESCALER_RANGE, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min = tmp.u32Minimum;
        max = tmp.u32Maximum;
        inc = tmp.u32Increment;

        return true;
    });

    framePrescaler.setFuncGet([this]() -> QVariant {
                                  UINT tmp = 0;
                                  if(is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_GET_FRAME_PRESCALER, &tmp, sizeof(tmp)) != IS_SUCCESS)
                                  {
                                      return {};
                                  }
                                  return tmp;
                              });
    framePrescaler.setFuncSet([this](const QVariant& v) -> bool {
        helper::AcquisitionGuard guard(this);
        UINT tmp = v.toUInt();
        return is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_SET_FRAME_PRESCALER, &tmp, sizeof(tmp)) == IS_SUCCESS;
    });
    framePrescaler.setRangeFunc([this](QVariant& min, QVariant& max, QVariant& inc) -> bool {
        RANGE_OF_VALUES_U32 tmp{};
        if(is_Trigger(this->m_hCamera, IS_TRIGGER_CMD_GET_FRAME_PRESCALER_RANGE, &tmp, sizeof(tmp)) != IS_SUCCESS)
        {
            return false;
        }
        min = tmp.u32Minimum;
        max = tmp.u32Maximum;
        inc = tmp.u32Increment;

        return true;
    });

    colorMode.setFuncGet([this]() -> QVariant {
        return is_SetColorMode(this->m_hCamera, IS_GET_COLOR_MODE);
    });
    colorMode.setFuncSet([this](const QVariant& v) -> bool {
        helper::AcquisitionGuard guard(this);
        return is_SetColorMode(this->m_hCamera, v.toInt());
    });

    colorConverter.setFuncGet([this]() -> QVariant {
        INT tmp;
        if (is_GetColorConverter(this->m_hCamera, this->colorMode().toInt(), &tmp, nullptr, nullptr) != IS_SUCCESS)
        {
            throw std::runtime_error("Error getting color converter.");
        }
        return tmp;
    });
    colorConverter.setFuncSet([this](const QVariant& v) -> bool {
        helper::AcquisitionGuard guard(this);

        return is_SetColorConverter(this->m_hCamera, IS_CM_ALL_POSSIBLE, v.toInt());
    });

    binning.setFuncGet([this]() -> QVariant {
        return is_SetBinning (this->m_hCamera, IS_GET_BINNING);
    });
    binning.setFuncSet([this](const QVariant& v) -> bool {
        helper::AcquisitionGuard guard(this);
        int ret = is_SetBinning (this->m_hCamera, v.toInt());
        binning.update();
        return ret;
    });
    supportedBinning.setFuncGet([this]() -> QVariant {
        return is_SetBinning (this->m_hCamera, IS_GET_SUPPORTED_BINNING);
    });

    subsampling.setFuncGet([this]() -> QVariant {
        return is_SetSubSampling (this->m_hCamera, IS_GET_SUBSAMPLING);
    });
    subsampling.setFuncSet([this](const QVariant& v) -> bool {
        helper::AcquisitionGuard guard(this);
        int ret = is_SetSubSampling (this->m_hCamera, v.toInt());
        subsampling.update();
        return ret;
    });
    supportedSubsampling.setFuncGet([this]() -> QVariant {
        return is_SetSubSampling (this->m_hCamera, IS_GET_SUPPORTED_SUBSAMPLING);
    });

    sensorScaler.setFuncSet([this](const QVariant& v) -> bool {
        helper::AcquisitionGuard guard(this);
        auto info = qvariant_cast<SENSORSCALERINFO>(v);
        INT mode = info.nCurrMode;
        double factor = info.dblCurrFactor;
        return is_SetSensorScaler (this->m_hCamera, static_cast<UINT>(mode), factor);
    });
    sensorScaler.setFuncGet([this]() -> QVariant {
        SENSORSCALERINFO info;
        if(is_GetSensorScalerInfo (this->m_hCamera, &info, sizeof(info)) != IS_SUCCESS)
        {
            memset(&info, 0, sizeof(info));
        }
        return QVariant::fromValue(info);
    });

    sensorInfo.setFuncGet([this]() -> QVariant {
        SENSORINFO info;
        if (is_GetSensorInfo(this->m_hCamera, &info) != IS_SUCCESS)
        {
            memset(&info, 0, sizeof(info));
        }

        return QVariant::fromValue(info);
    });

    bHardwareGammaSet.setFuncGet([this]() -> bool {
        return is_SetHardwareGamma (this->m_hCamera, IS_GET_HW_GAMMA) == IS_SET_HW_GAMMA_ON;
    });

    bGainBoostSet.setFuncGet(
        [this]() -> bool { return is_SetGainBoost(this->m_hCamera, IS_GET_GAINBOOST) == IS_SET_GAINBOOST_ON; });
}

void Camera::initAutoFuncs()
{
    // todo xs, xc

    useSensorAutoFunctions.useSensorAutoGain.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int ret = is_SetAutoParameter(this->handle(), IS_GET_ENABLE_AUTO_SENSOR_GAIN, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    useSensorAutoFunctions.useSensorAutoGain.setFuncSet([this](const QVariant& v) -> bool {
        if (v.toBool() && !useSensorAutoFunctions.useSensorAutoGain.isSupported())
        {
            useSensorAutoFunctions.useSensorAutoGain = false;
            return false;
        }
        return true;
    });

    useSensorAutoFunctions.useSensorAutoGain.setFuncSupported([this]() -> bool {
        UEYE_AUTO_INFO info{};
        if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
            return (info.AutoAbility & (AC_SENSOR_GAIN | AC_SENSOR_GAIN_SHUTTER)) > 0;
        return false;
    });

    useSensorAutoFunctions.useSensorAutoExposure.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int ret = is_SetAutoParameter(this->handle(), IS_GET_ENABLE_AUTO_SENSOR_SHUTTER, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    useSensorAutoFunctions.useSensorAutoExposure.setFuncSet([this](const QVariant& v) -> bool {
        if (v.toBool() && !useSensorAutoFunctions.useSensorAutoExposure.isSupported())
        {
            useSensorAutoFunctions.useSensorAutoExposure = false;
            return false;
        }
        return true;
    });

    useSensorAutoFunctions.useSensorAutoExposure.setFuncSupported([this]() -> bool {
        UEYE_AUTO_INFO info{};
        if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
            return (info.AutoAbility & (AC_SENSOR_SHUTTER | AC_SENSOR_GAIN_SHUTTER)) > 0;
        return false;
    });

    useSensorAutoFunctions.useSensorWhiteBalance.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int ret = is_SetAutoParameter(this->handle(), IS_GET_ENABLE_AUTO_SENSOR_WHITEBALANCE, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    useSensorAutoFunctions.useSensorWhiteBalance.setFuncSet([this](const QVariant& v) -> bool {
        if (v.toBool() && !useSensorAutoFunctions.useSensorWhiteBalance.isSupported())
        {
            useSensorAutoFunctions.useSensorWhiteBalance = false;
            return false;
        }
        return true;
    });

    useSensorAutoFunctions.useSensorWhiteBalance.setFuncSupported([this]() -> bool {
        UEYE_AUTO_INFO info{};
        if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
            return (info.AutoAbility & AC_SENSOR_WB) == AC_SENSOR_WB;
        return false;
    });

    useSensorAutoFunctions.useSensorAutoFramerate.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int ret = is_SetAutoParameter(this->handle(), IS_GET_ENABLE_AUTO_SENSOR_FRAMERATE, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    useSensorAutoFunctions.useSensorAutoFramerate.setFuncSet([this](const QVariant& v) -> bool {
        if (v.toBool() && !useSensorAutoFunctions.useSensorAutoFramerate.isSupported())
        {
            useSensorAutoFunctions.useSensorAutoFramerate = false;
            return false;
        }
        return true;
    });

    useSensorAutoFunctions.useSensorAutoFramerate.setFuncSupported([this]() -> bool {
        UEYE_AUTO_INFO info{};
        if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
            return (info.AutoAbility & (AC_SENSOR_FRAMERATE)) == AC_SENSOR_FRAMERATE;
        return false;
    });

    autoGain.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int cmd = IS_GET_ENABLE_AUTO_GAIN;

        if (useSensorAutoFunctions.useSensorAutoGain.value().toBool())
        {
            cmd = IS_GET_ENABLE_AUTO_SENSOR_GAIN;
        }

        if (isXS() && !isXC())
        {
            cmd = IS_GET_ENABLE_AUTO_SENSOR_GAIN_SHUTTER;
        }

        int ret = is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    autoGain.setFuncSet([this](const QVariant& m) -> int {
        double dblEnable = m.toDouble();

        int cmd = IS_SET_ENABLE_AUTO_GAIN;
        if (useSensorAutoFunctions.useSensorAutoGain.value().toBool())
        {
            cmd = IS_SET_ENABLE_AUTO_SENSOR_GAIN;
        }
        if (isXS() && !isXC())
        {
            cmd = IS_SET_ENABLE_AUTO_SENSOR_GAIN_SHUTTER;
        }

        int ret = is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);

        if (isXS())
        {
            autoExposure.mean.update();
        }
        return ret;
    });

    autoGain.setFuncSupported([this]() -> bool {
        if (autoFramerate.value().toBool())
        {
            return false;
        }

        if (!this->hasMasterGain())
        {
            return false;
        }

        UEYE_AUTO_INFO info{};
        if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
        {
            if ((info.AutoAbility & (AC_GAIN | AC_SENSOR_GAIN | AC_SENSOR_GAIN_SHUTTER)) > 0)
            {
                return true;
            }
        }
        return false;
    });

    autoWhiteBalance.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int cmd = IS_GET_ENABLE_AUTO_WHITEBALANCE;

        if (useSensorAutoFunctions.useSensorWhiteBalance.value().toBool())
        {
            cmd = IS_GET_ENABLE_AUTO_SENSOR_WHITEBALANCE;
        }

        int ret = is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    autoWhiteBalance.setFuncSet([this](const QVariant& m) -> int {
        double dblEnable = m.toDouble();

        int cmd = IS_SET_ENABLE_AUTO_WHITEBALANCE;
        if (useSensorAutoFunctions.useSensorWhiteBalance.value().toBool())
        {
            cmd = IS_SET_ENABLE_AUTO_SENSOR_WHITEBALANCE;
        }

        return is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);
    });

    autoWhiteBalance.setFuncSupported([this]() -> bool {
        if (this->hasRGain() && this->hasBGain() && this->hasGGain())
        {
            UEYE_AUTO_INFO info{};
            if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
            {
                if ((info.AutoAbility & AC_WHITEBAL) == AC_WHITEBAL)
                {
                    return true;
                }
            }
        }

        return false;
    });

    autoFramerate.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int cmd = IS_GET_ENABLE_AUTO_FRAMERATE;

        if (useSensorAutoFunctions.useSensorAutoFramerate.value().toBool())
        {
            cmd = IS_GET_ENABLE_AUTO_SENSOR_FRAMERATE;
        }

        int ret = is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    autoFramerate.setFuncSet([this](const QVariant& m) -> int {
        double dblEnable = m.toDouble();

        int cmd = IS_SET_ENABLE_AUTO_FRAMERATE;
        if (useSensorAutoFunctions.useSensorAutoFramerate.value().toBool())
        {
            cmd = IS_SET_ENABLE_AUTO_SENSOR_FRAMERATE;
        }

        return is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);
    });

    autoFramerate.setFuncSupported([this]() -> bool {
        if (autoExposure.mean.value().toBool() && !autoGain.value().toBool())
        {
            UEYE_AUTO_INFO info{};
            if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
            {
                if ((info.AutoAbility & AC_FRAMERATE) == AC_FRAMERATE)
                {
                    return true;
                }
            }
        }
        return false;
    });

    autoExposure.mean.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int cmd = IS_GET_ENABLE_AUTO_SHUTTER;

        if (useSensorAutoFunctions.useSensorAutoExposure.value().toBool())
        {
            cmd = IS_GET_ENABLE_AUTO_SENSOR_SHUTTER;
        }

        if (isXS() && !isXC())
        {
            cmd = IS_GET_ENABLE_AUTO_SENSOR_GAIN_SHUTTER;
        }

        int ret = is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    autoExposure.mean.setFuncSet([this](const QVariant& m) -> int {
        autoExposure.usePeakAutoImpl = !m.toBool();

        double dblEnable = m.toDouble();

        if (!m.toBool() && autoFramerate.value().toBool())
        {
            autoFramerate.setValue(m);
        }

        int cmd = IS_SET_ENABLE_AUTO_SHUTTER;
        if (useSensorAutoFunctions.useSensorAutoExposure.value().toBool())
        {
            cmd = IS_SET_ENABLE_AUTO_SENSOR_SHUTTER;
        }

        if (isXS() && !isXC())
        {
            cmd = IS_SET_ENABLE_AUTO_SENSOR_GAIN_SHUTTER;
        }

        return is_SetAutoParameter(this->handle(), cmd, &dblEnable, nullptr);
    });

    autoExposure.mean.setFuncSupported([this]() -> bool {
        UEYE_AUTO_INFO info{};
        if (is_GetAutoInfo(this->handle(), &info) == IS_SUCCESS)
        {
            if ((info.AutoAbility & (AC_SHUTTER | AC_SENSOR_SHUTTER | AC_SENSOR_GAIN_SHUTTER)) > 0)
            {
                return true;
            }
        }
        return false;
    });

    autoExposure.peak.peakConfiguration.setFuncGet([this]() -> QVariant {
        if (useSensorAutoFunctions.useSensorAutoExposure.value().toBool())
        {
            // peak is not supported with sensor functions
            return {};
        }
        AES_PEAK_CONFIGURATION peakConfig{};

        QSharedPointer<char> tmp(new char[CONFIG_SIZE], [](const char* x) { delete[] x; });
        auto config = reinterpret_cast<AES_CONFIGURATION*>(tmp.data());
        config->nMode = IS_AES_MODE_PEAK;

        int ret = is_AutoParameter(this->handle(), IS_AES_CMD_GET_CONFIGURATION, config, CONFIG_SIZE);

        if (ret == IS_SUCCESS)
        {
            memcpy(&peakConfig, &config->pConfiguration, sizeof(AES_PEAK_CONFIGURATION));
        }
        return QVariant::fromValue(peakConfig);
    });

    autoExposure.peak.peakConfiguration.setFuncSet([this](const QVariant& v) -> int {
        if (useSensorAutoFunctions.useSensorAutoExposure.value().toBool())
        {
            // peak is not supported with sensor functions
            return 0;
        }
        auto peakConfig = qvariant_cast<AES_PEAK_CONFIGURATION>(v);

        QSharedPointer<char> tmp(new char[CONFIG_SIZE], [](const char* x) { delete[] x; });
        auto config = reinterpret_cast<AES_CONFIGURATION*>(tmp.data());
        config->nMode = IS_AES_MODE_PEAK;

        memcpy(&config->pConfiguration, &peakConfig, sizeof(AES_PEAK_CONFIGURATION));

        return is_AutoParameter(this->handle(), IS_AES_CMD_SET_CONFIGURATION, config, CONFIG_SIZE);
    });

    autoExposure.peak.peakRange.setFuncGet([this]() -> QVariant {
        if (useSensorAutoFunctions.useSensorAutoExposure.value().toBool())
        {
            // peak is not supported with sensor functions
            return {};
        }
        AES_PEAK_CONFIGURATION_RANGE peakConfig{};

        QSharedPointer<char> tmp(new char[RANGE_SIZE], [](const char* x) { delete[] x; });
        auto config = reinterpret_cast<AES_CONFIGURATION*>(tmp.data());
        config->nMode = IS_AES_MODE_PEAK;

        int ret = is_AutoParameter(this->handle(), IS_AES_CMD_GET_CONFIGURATION_RANGE, config, RANGE_SIZE);

        if (ret == IS_SUCCESS)
        {
            memcpy(&peakConfig, &config->pConfiguration, sizeof(AES_PEAK_CONFIGURATION_RANGE));
        }
        return QVariant::fromValue(peakConfig);
    });

    autoExposure.peak.peakRange.setFuncSet([this](const QVariant& v) -> int {
        if (useSensorAutoFunctions.useSensorAutoExposure.value().toBool())
        {
            // peak is not supported with sensor functions
            return 0;
        }
        auto peakConfig = qvariant_cast<AES_PEAK_CONFIGURATION_RANGE>(v);

        QSharedPointer<char> tmp(new char[CONFIG_SIZE], [](const char* x) { delete[] x; });
        auto config = reinterpret_cast<AES_CONFIGURATION*>(tmp.data());
        config->nMode = IS_AES_MODE_PEAK;

        memcpy(&config->pConfiguration, &peakConfig, sizeof(AES_PEAK_CONFIGURATION_RANGE));

        return is_AutoParameter(this->handle(), IS_AES_CMD_SET_CONFIGURATION, config, CONFIG_SIZE);
    });

    autoExposure.peak.setFuncGet([this]() -> INT {
        INT nEnabled = IS_AUTOPARAMETER_DISABLE;
        is_AutoParameter(this->handle(), IS_AES_CMD_GET_ENABLE, &nEnabled, sizeof(nEnabled));
        return nEnabled;
    });

    autoExposure.peak.setFuncSet([this](const QVariant& v) -> int {
        INT nEnabled = v.toInt();
        return is_AutoParameter(this->handle(), IS_AES_CMD_SET_ENABLE, &nEnabled, sizeof(nEnabled));
    });

    autoExposure.peak.setFuncSupported([this]() -> bool {
        INT supportedModes = 0;
        int ret = is_AutoParameter(
                this->handle(), IS_AES_CMD_GET_SUPPORTED_TYPES, &supportedModes, sizeof(supportedModes));
        if (ret == IS_SUCCESS)
        {
            if (supportedModes & IS_AES_MODE_PEAK)
            {
                return true;
            }
        }
        return false;
    });

    runAutoExposureAndAutoGainOnce.setFuncGet([this]() -> bool {
        double dblEnable = 0.0;
        int ret = is_SetAutoParameter(this->handle(), IS_GET_AUTO_BRIGHTNESS_ONCE, &dblEnable, nullptr);
        return ret == IS_SUCCESS && static_cast<bool>(dblEnable);
    });

    runAutoExposureAndAutoGainOnce.setFuncSet([this](const QVariant& v) -> int {
      double dblEnable = v.toDouble();
      return is_SetAutoParameter(this->handle(), IS_SET_AUTO_BRIGHTNESS_ONCE, &dblEnable, nullptr);
    });
}


int Camera::Open(UEYE_CAMERA_INFO camera, bool allowUpload)
{
    m_hCamera = camera.dwDeviceID | IS_USE_DEVICE_ID;
    m_camInfo = camera;
    if(allowUpload)
    {
        m_hCamera |= IS_ALLOW_STARTER_FW_UPLOAD;
    }
    auto nRet = is_InitCamera(&m_hCamera, nullptr);
    if(nRet != IS_SUCCESS)
    {
        return nRet;
    }

    //is_Configuration(IS_CONFIG_INITIAL_PARAMETERSET_CMD_GET, &uInitialParameterSet, sizeof(unsigned int));

    nRet = is_GetCameraInfo(m_hCamera, &m_boardInfo);
    if(nRet != IS_SUCCESS)
    {
        return nRet;
    }

    m_memoryModeEn = checkMemoryMode();

    UINT capabilities;
    nRet = is_Focus(m_hCamera, FOC_CMD_GET_CAPABILITIES, &capabilities, sizeof(capabilities));
    hasFocus = nRet == IS_SUCCESS &&
               (capabilities & FOC_CAP_AUTOFOCUS_SUPPORTED || capabilities & FOC_CAP_MANUAL_SUPPORTED);


    m_pEvDevice->start(m_hCamera, {IS_SET_EVENT_FRAME, IS_SET_EVENT_CAPTURE_STATUS, IS_SET_EVENT_PMC_IMAGE_PARAMS_CHANGED, IS_SET_EVENT_DEVICE_RECONNECTED});

    updateAll();

    m_supportedTriggerModes = is_SetExternalTrigger(m_hCamera, IS_GET_SUPPORTED_TRIGGER_MODE);

    for(auto& test : {IS_SET_TRIGGER_SOFTWARE,
                       IS_SET_TRIGGER_HI_LO,
                       IS_SET_TRIGGER_LO_HI,
                      IS_SET_TRIGGER_SOFTWARE &~ IS_SET_TRIGGER_CONTINUOUS,
                       IS_SET_TRIGGER_HI_LO &~ IS_SET_TRIGGER_CONTINUOUS,
                       IS_SET_TRIGGER_LO_HI &~ IS_SET_TRIGGER_CONTINUOUS})
    {
        if((m_supportedTriggerModes & test) == test)
        {
            m_setTriggerMode = test;
            break;
        }
    }

    return IS_SUCCESS;
}

void Camera::Close()
{
    m_pEvDevice->stop();
    m_pEvDevice->wait();

    freeImages();

    is_ExitCamera(m_hCamera);
    m_hCamera = 0;
    m_camInfo = {};
    m_AutoInfo = {};
    hasFocus = false;
}

bool Camera::isOpen() const
{
    return m_hCamera != 0;
}

bool Camera::isLive() const
{
    return is_CaptureVideo (m_hCamera, IS_GET_LIVE);
}

double Camera::framesPerSeconds() const
{
    double current_fps;
    is_GetFramesPerSecond(m_hCamera, &current_fps);
    return current_fps;
}

bool Camera::inUse() const
{
    return m_camInfo.dwInUse;
}

void Camera::saveParameterSet() const
{
    is_ParameterSet(getCameraHandle(), IS_PARAMETERSET_CMD_SAVE_EEPROM, nullptr, 0);
}

void Camera::saveParameterSet(const QString& file)
{
    is_ParameterSet(getCameraHandle(), IS_PARAMETERSET_CMD_SAVE_FILE, const_cast<wchar_t*>(file.toStdWString().c_str()), 0 );
}

void Camera::readParameterSet(const QString& file)
{
    helper::AcquisitionGuard guard(this);

    auto ret = is_ParameterSet(getCameraHandle(), IS_PARAMETERSET_CMD_LOAD_FILE, const_cast<wchar_t*>(file.toStdWString().c_str()), 0 );
    if (ret != IS_SUCCESS)
    {
        /* HANDLE ERROR */
#if 0
        if (ret == IS_INVALID_CAMERA_TYPE)
        {
            QMessageBox::information (this, tr("Load camera parameter file"), QString(tr("The configuration file\n%1\ncannot be applied to the selected camera!")).arg(fileName), 0);
        }
        else
        {
            QString msg = QString(tr("Error loading camerafile (error %1)")).arg(ret);
            QMessageBox::warning (this, tr("Load camera parameter file"), msg);
        }
#endif
    }

    updateAll();

    // Update trigger settings, as these may have changed
    auto value = m_triggerMode.value().toUInt();
    if(value != IS_TRIGGER_SOURCE_OFF)
    {
        m_setTriggerMode = value;
        guard.setTriggered(true);
    }
    else
    {
        guard.setTriggered(false);
    }

    parameterChanged();
}

void Camera::readParameterSet()
{
    helper::AcquisitionGuard guard(this);

    is_ParameterSet(getCameraHandle(), IS_PARAMETERSET_CMD_LOAD_EEPROM, nullptr, 0 );

    updateAll();

    // Update trigger settings, as these may have changed
    auto value = m_triggerMode.value().toUInt();
    if(value != IS_TRIGGER_SOURCE_OFF)
    {
        m_setTriggerMode = value;
        guard.setTriggered(true);
    }
    else
    {
        guard.setTriggered(false);
    }

    parameterChanged();
}

void Camera::resetToDefaults()
{
    helper::AcquisitionGuard guard(this);
    guard.setTriggered(false);
    m_triggerMode.setValue(IS_TRIGGER_SOURCE_OFF);

    is_ResetToDefault(getCameraHandle());

    updateAll();

    // Update trigger settings, as these may have changed
    auto value = m_triggerMode.value().toUInt();
    if(value != IS_TRIGGER_SOURCE_OFF)
    {
        m_setTriggerMode = value;
        guard.setTriggered(true);
    }
    else
    {
        guard.setTriggered(false);
    }

    parameterChanged();
}

double Camera::GetAverageBandwidth()
{
    double dblFramerate = fps.value().toDouble();
    INT iWidth = aoi.size().width();
    INT iHeight = aoi.size().height();
    double dblRet;

    {
        double min, max, intervall;
        is_GetFrameTimeRange(getCameraHandle(), &min, &max, &intervall );

        // Add or subtract a small value to avoid rounding errors
        double dblMinFramerate = (1.0 / max) - 0.000000001;
        double dblMaxFramerate = (1.0 / min) + 0.000000001;

        if((dblFramerate < dblMinFramerate) || (dblFramerate > dblMaxFramerate))
        {
            dblFramerate = 0;
        }

        double mult = 1.0;
        auto cm = colorMode().toInt();
        auto cv = colorConverter().toInt();

        if(cv != IS_CONV_MODE_HARDWARE_3X3)
        {
            if ((cm == IS_CM_MONO10)                                                  ||
                (cm == (IS_CM_MONO10 | IS_CM_PREFER_PACKED_SOURCE_FORMAT))            ||
                (cm == IS_CM_MONO12)                                                  ||
                /* The format is only packed in the destination buffer, the source is RAW12 unpacked */
                (cm == IS_CM_BGR10_PACKED)                                            ||
                (cm == IS_CM_BGR10_UNPACKED)                                          ||
                (cm == (IS_CM_BGR10_UNPACKED | IS_CM_PREFER_PACKED_SOURCE_FORMAT))    ||
                (cm == IS_CM_BGR12_UNPACKED))
            {
                /* These formats always use 10 or 12 bit raw. The raw format needs 16 bits */
                mult = 2.0;
            }
        }
        else
        {
            mult = ((getBitsPerPixel(cm) + 7) / 8);
        }

        //bool test = IsEthernet(m_hCam);
        // The bandwidth differs for each interface
        if (is_GetCameraType(getCameraHandle()) & IS_INTERFACE_TYPE_USB3)
        {
            if (isCameraMemoryEnabled())
            {
                /* u3v leader + trailer */
                const unsigned int u3vLeaderSize = 56;
                const unsigned int u3vTrailerSize = 32;

                dblRet = iWidth * iHeight * dblFramerate * mult + (dblFramerate * (u3vLeaderSize + u3vTrailerSize));
            }
            else
            {
                dblRet = iWidth * iHeight * dblFramerate * mult;
            }
        }
        else if((is_GetCameraType(getCameraHandle()) & IS_INTERFACE_TYPE_USB) != 0)
        {
            dblRet = iWidth * iHeight * dblFramerate * mult * 1.16;
        }
        else
        {
            dblRet = iWidth * iHeight * dblFramerate * mult * 1.035;
        }

        // This color mode transfers 2 byte
        if (cm == IS_COLORMODE_CBYCRY)
        {
            dblRet *= 2;
        }

        dblRet /= (1024. * 1024.);

        if ((cm == (IS_CM_SENSOR_RAW10    | IS_CM_PREFER_PACKED_SOURCE_FORMAT))   ||
            (cm == (IS_CM_MONO10          | IS_CM_PREFER_PACKED_SOURCE_FORMAT))   ||
            (cm == (IS_CM_BGR10_UNPACKED  | IS_CM_PREFER_PACKED_SOURCE_FORMAT)))
        {
            dblRet *= (2.0 / 3.0);
        }
    }

    return dblRet;
}


double Camera::GetCameraPeakBandwidth()
{
    if (is_GetCameraType(getCameraHandle()) & IS_CAMERA_TYPE_UEYE_ETH)
    {
        return GetAverageBandwidth();
    }

    INT mult = 1;
    auto cm = colorMode().toInt();
    auto cv = colorConverter().toInt();

    if(cv != IS_CONV_MODE_HARDWARE_3X3)
    {
        /* Set the number of bytes per pixel and color */
        if ((cm == IS_CM_MONO10)                                                  ||
            (cm == (IS_CM_MONO10 | IS_CM_PREFER_PACKED_SOURCE_FORMAT))            ||
            (cm == IS_CM_MONO12)                                                  ||
            /* The format is only packed in the destination buffer, the source is RAW12 unpacked */
            (cm == IS_CM_BGR10_PACKED)                                            ||
            (cm == IS_CM_BGR10_UNPACKED)                                          ||
            (cm == (IS_CM_BGR10_UNPACKED | IS_CM_PREFER_PACKED_SOURCE_FORMAT))    ||
            (cm == IS_CM_BGR12_UNPACKED))
        {
            mult = 2;
        }
        else
        {
            mult = 1;
        }
    }
    else
    {
        mult = ((getBitsPerPixel(cm) + 7) / 8);
    }

    // This color mode transfers 2 byte
    if (cm == IS_COLORMODE_CBYCRY)
    {
        mult *= 2;
    }

    /* Use reduced Bandwidth for actively packed formats */
    double dPeakMult = 1.0;
    if (cm & IS_CM_PREFER_PACKED_SOURCE_FORMAT)
    {
        dPeakMult = (2.0 / 3.0);
    }

    /* For XS1 and XS2 the bytes transsferred with each pixelclock is 1 byte independed of the color format used */
    if (isXS())
    {
        mult = 1;
    }

    /* For USB use 116% of calculated peak (for overhead) */
    if (is_GetCameraType(getCameraHandle()) & IS_INTERFACE_TYPE_USB3)
    {
        return ((double) pixelclock.value().toInt()) * mult * dPeakMult;
    }
    if(is_GetCameraType(getCameraHandle()) & IS_CAMERA_TYPE_UEYE_USB)
    {
        return ((double) pixelclock.value().toInt()) * 1.16 * mult * dPeakMult;
    }
    else /* For ETH use 103.5% of calculated peak (for overhead) */
    {
        return ((double) pixelclock.value().toInt()) * 1.035 * mult * dPeakMult;
    }
}


bool Camera::isXS() const
{
    const auto sensorID{qvariant_cast<SENSORINFO>(sensorInfo()).SensorID};

    return ((IS_SENSOR_XS == sensorID) ||
            (IS_SENSOR_UI1008_C == sensorID) ||
            (IS_SENSOR_UI1013XC == sensorID) ||
            (IS_SENSOR_XS_R2 == sensorID));
}

bool Camera::isXC() const
{
    return (IS_SENSOR_UI1013XC == qvariant_cast<SENSORINFO>(sensorInfo()).SensorID);
}

int Camera::getSID() const
{
    return qvariant_cast<SENSORINFO>(sensorInfo()).SensorID;
}

bool Camera::isColor() const
{
    return qvariant_cast<SENSORINFO>(sensorInfo()).nColorMode != IS_COLORMODE_MONOCHROME;
}

bool Camera::isGlobalShutter() const
{
    return qvariant_cast<SENSORINFO>(sensorInfo()).bGlobShutter;
}

bool Camera::hasMasterGain() const
{
    return qvariant_cast<SENSORINFO>(sensorInfo()).bMasterGain;
}

bool Camera::hasRGain() const
{
    return qvariant_cast<SENSORINFO>(sensorInfo()).bRGain;
}

bool Camera::hasGGain() const
{
    return qvariant_cast<SENSORINFO>(sensorInfo()).bGGain;
}

bool Camera::hasBGain() const
{
    return qvariant_cast<SENSORINFO>(sensorInfo()).bBGain;
}

HIDS Camera::handle() const
{
    return m_hCamera;
}

std::pair<int, int> Camera::GetMaxImageSize() const
{
    const auto info{qvariant_cast<SENSORINFO>(sensorInfo())};

    return std::make_pair(info.nMaxWidth, info.nMaxHeight);
}

QString Camera::SerNo() const
{
    return QString::fromLatin1(m_camInfo.SerNo);
}

QString Camera::Model() const
{
    return QString::fromLatin1(m_camInfo.Model);
}

QString Camera::WindowTitle() const
{
    return QString(tr("%1 | Cam. ID: %2 | Ser. No.: %3")).arg(Model(), QString::number(getCameraID()), SerNo());
}

int Camera::getDeviceID() const
{
    return static_cast<int>(m_camInfo.dwDeviceID);
}

int Camera::getCameraID() const
{
    return static_cast<int>(m_camInfo.dwCameraID);
}

void Camera::updateAll()
{
    exposure.update();
    fps.update();
    pixelclock.update();

    bMaxExposure.update();
    bdualExposure.update();

    aoi.update();

    bColor.update();
    bJPEG.update();

    bHasAutoBlackLevel.update();
    bHasManualBlackLevel.update();
    bHasHardwareGamma.update();
    bHasSoftwareGamma.update();
    bHasGainBoost.update();

    bSoftwareGammaSet.update();
    bHardwareGammaSet.update();
    bGainBoostSet.update();

    MasterGain.update();
    softwareGamma.update();

    BlackLevel.update();

    RedGain.update();
    GreenGain.update();
    BlueGain.update();

    bAutoBlackLevel.update();

    TriggerDelay.update();
    bEnableTriggerDelaySet.update();

    FlashDelay.update();

    FlashDuration.update();

    pwm.update();

    bHasEdgeEnhancement.update();
    edgeEnhancement.update();

    focusOnceActive.update();
    bXsHasBacklightComp.update();
    bXsHasAES.update();
    bXsHasAGS.update();
    bXsHasAGES.update();

    xsJPEGCompression.update();
    xsSharpness.update();
    xsSaturation.update();
    xsLSCDefaultValue.update();

    manualFocus.update();
    autoFocus.update();


    vFactor.update();
    hFactor.update();

    bAntiAliasingSupported.update();
    scalerActive.update();
    scalerAntiAliasingActive.update();

    m_triggerMode.update();

    colorMode.update();
    colorConverter.update();

    binning.update();
    supportedBinning.update();
    subsampling.update();
    supportedSubsampling.update();

    sensorScaler.update();

    sensorInfo.update();

    useSensorAutoFunctions.useSensorAutoFramerate.update();
    useSensorAutoFunctions.useSensorWhiteBalance.update();
    useSensorAutoFunctions.useSensorAutoExposure.update();
    useSensorAutoFunctions.useSensorAutoGain.update();

    autoGain.update();
    autoWhiteBalance.update();
    autoFramerate.update();
    autoExposure.mean.update();
    autoExposure.peak.update();
}

bool Camera::isCameraMemoryEnabled() const
{
    return m_memoryModeEn;
}

bool Camera::hasXsAES() const
{
    return false;
}

bool Camera::hasXsAGES() const
{
    return true;
}

UEYE_CAMERA_INFO Camera::getCamInfo() const
{
    return m_camInfo;
}

bool Camera::checkMemoryMode() const
{
    if (isETH() || isPEAK())
    {
        return true;
    }
    else if (m_boardInfo.Type == IS_BOARD_TYPE_UEYE_USB3_SE)
    {
        return true;
    }
    else if (m_boardInfo.Type == IS_BOARD_TYPE_UEYE_USB3_CP)
    {
        int nSupportedModes = 0;
        is_DeviceFeature(m_hCamera, IS_DEVICE_FEATURE_CMD_GET_SUPPORTED_FEATURES,
                &nSupportedModes, sizeof(nSupportedModes));

        if (nSupportedModes & IS_DEVICE_FEATURE_CAP_MEMORY_MODE)
        {
            INT enabled = 0;
            if(is_DeviceFeature(m_hCamera, IS_DEVICE_FEATURE_CMD_GET_MEMORY_MODE_ENABLE, &enabled, sizeof(enabled)) == IS_SUCCESS)
            {
                return enabled;
            }
        }
    }

    return false;
}

Camera::~Camera()
{
    if (isOpen())
    {
        Close();
    }
}

UEYE_IMAGE Camera::getImage(const char* pbuf)
{
    for (auto& image : m_Images)
    {
        if (image.pBuf == pbuf)
        {
            return image;
        }
    }

    throw std::runtime_error("Wtf?");
}

void Camera::freeImages()
{
    auto result = 0;
    for (int i = 0; i < 100; ++i)
    {
        result = is_ClearSequence (getCameraHandle());
        if (result != IS_SEQ_BUFFER_IS_LOCKED)
        {
            break;
        }

        /* buffer can be locked from image processing engine so we wait and try again */
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (auto& image : m_Images)
    {
        if (nullptr != image.pBuf)
        {
            is_FreeImageMem (getCameraHandle(), image.pBuf, image.nImageID);
        }

        image.pBuf = nullptr;
        image.nImageID = 0;
        image.nImageSeqNum = 0;
    }
}

void Camera::emptyImages()
{
    for (auto& image : m_Images)
    {
        if (image.pBuf)
        {
            is_LockSeqBuf(getCameraHandle(), image.nImageSeqNum, image.pBuf);
            ZeroMemory (image.pBuf, image.nBufferSize);
            is_UnlockSeqBuf (getCameraHandle(), image.nImageSeqNum, image.pBuf);
        }
    }
}

bool Camera::allocImages(const sBufferProps &buffer_property)
{
    int nWidth = 0;
    int nHeight = 0;

    UINT nAbsPosX;
    UINT nAbsPosY;

    int ret = is_AOI(getCameraHandle(), IS_AOI_IMAGE_GET_POS_X_ABS, reinterpret_cast<void*>(&nAbsPosX) , sizeof(nAbsPosX));
    if (ret == IS_SUCCESS)
    {
        is_AOI(getCameraHandle(), IS_AOI_IMAGE_GET_POS_Y_ABS, reinterpret_cast<void*>(&nAbsPosY), sizeof(nAbsPosY));
    }

    freeImages();

    for (unsigned int i = 0; i < sizeof(m_Images) / sizeof(m_Images[0]); i++)
    {
        m_Images[i].buffer_property = buffer_property;

        nWidth = m_Images[i].buffer_property.width;
        nHeight = m_Images[i].buffer_property.height;

        if (ret == IS_SUCCESS && nAbsPosX == IS_AOI_IMAGE_POS_ABSOLUTE)
        {
            m_Images[i].buffer_property.width = nWidth = static_cast<int>(GetMaxImageSize().first);
        }
        if (ret == IS_SUCCESS && nAbsPosY == IS_AOI_IMAGE_POS_ABSOLUTE)
        {
            m_Images[i].buffer_property.height = nHeight = static_cast<int>(GetMaxImageSize().second);
        }

        if (is_AllocImageMem (getCameraHandle(), nWidth, nHeight, m_Images[i].buffer_property.bitspp, &m_Images[i].pBuf,
                              &m_Images[i].nImageID) != IS_SUCCESS)
            return FALSE;

        if (is_AddToSequence (getCameraHandle(), m_Images[i].pBuf, m_Images[i].nImageID) != IS_SUCCESS)
            return FALSE;

        m_Images[i].nImageSeqNum = static_cast<int>(i) + 1;
        m_Images[i].nBufferSize = nWidth * nHeight * m_Images[i].buffer_property.bitspp / 8;
        memset(m_Images[i].pBuf, 0xFF, m_Images[i].nBufferSize);

    }

    return TRUE;
}

int Camera::SetupCapture()
{
    int width, height;
    // init the memorybuffer properties
    IS_RECT rectAOI;
    INT nRet = is_AOI(getCameraHandle(), IS_AOI_IMAGE_GET_AOI, reinterpret_cast<void*>(&rectAOI), sizeof(rectAOI));

    if (nRet == IS_SUCCESS)
    {
        width  = rectAOI.s32Width;
        height = rectAOI.s32Height;

        // get current colormode
        int colormode = is_SetColorMode(getCameraHandle(), IS_GET_COLOR_MODE);

        sBufferProps buffer_property{};
        // fill memorybuffer properties
        buffer_property.width  = width;
        buffer_property.height = height;
        buffer_property.colorformat = colormode;
        buffer_property.bitspp = getBitsPerPixel(colormode);

        switch (colormode)
        {
        default:
        case IS_CM_MONO8:
        case IS_CM_SENSOR_RAW8:
            buffer_property.pRgbTable = m_table;
            buffer_property.tableentries = 256;
            buffer_property.imgformat = QImage::Format_Indexed8;
            break;
        case IS_CM_BGR565_PACKED:
            buffer_property.imgformat = QImage::Format_RGB16;
            break;
        case IS_CM_RGB8_PACKED:
        case IS_CM_BGR8_PACKED:
            buffer_property.imgformat = QImage::Format_RGB888;
            break;
        case IS_CM_RGBA8_PACKED:
        case IS_CM_BGRA8_PACKED:
            buffer_property.imgformat = QImage::Format_RGB32;
            break;
        }

        // Reallocate image buffers
        allocImages(buffer_property);
    }
    return 0;
}

int Camera::searchDefImageFormats(int suppportMask) const
{
    int ret = IS_SUCCESS;
    int nNumber;
    int format = 0;
    IMAGE_FORMAT_LIST *pFormatList;
    IS_RECT rectAOI;

    if ((ret=is_ImageFormat(getCameraHandle(), IMGFRMT_CMD_GET_NUM_ENTRIES, reinterpret_cast<void*>(&nNumber), sizeof(nNumber))) == IS_SUCCESS &&
        (ret=is_AOI(getCameraHandle(), IS_AOI_IMAGE_GET_AOI, reinterpret_cast<void*>(&rectAOI), sizeof(rectAOI))) == IS_SUCCESS)
    {
        int i = 0;
        size_t nSize = sizeof(IMAGE_FORMAT_LIST) + (static_cast<size_t>(nNumber) - 1) * sizeof(IMAGE_FORMAT_LIST);
        pFormatList = reinterpret_cast<IMAGE_FORMAT_LIST*>(new char[nSize]);
        pFormatList->nNumListElements = static_cast<UINT>(nNumber);
        pFormatList->nSizeOfListEntry = sizeof(IMAGE_FORMAT_INFO);

        if((ret=is_ImageFormat(getCameraHandle(), IMGFRMT_CMD_GET_LIST, reinterpret_cast<void*>(pFormatList), static_cast<UINT>(nSize))) == IS_SUCCESS)
        {
            for(i=0; i<nNumber; i++)
            {
                if ((pFormatList->FormatInfo[i].nSupportedCaptureModes & static_cast<UINT>(suppportMask)) &&
                     pFormatList->FormatInfo[i].nHeight == static_cast<UINT>(rectAOI.s32Height) &&
                     pFormatList->FormatInfo[i].nWidth  == static_cast<UINT>(rectAOI.s32Width))
                {
                    format = pFormatList->FormatInfo[i].nFormatID;
                    break;
                }
            }
        }
        else
        {
            qDebug("error: is_ImageFormat returned %d", ret);
        }

        delete[] pFormatList;
    }
    else
    {
        qDebug("error: is_ImageFormat returned %d", ret);
    }
    return format;
}

bool Camera::hasAWB() const
{
    return isColor() && ((m_AutoInfo.AutoAbility & AC_WHITEBAL) == AC_WHITEBAL);
}

void Camera::forceTrigger() const {
    is_ForceTrigger(handle());
}

bool Camera::isUSB2() const
{
    return (m_boardInfo.Type & IS_INTERFACE_MASK) == IS_INTERFACE_TYPE_USB;
}

bool Camera::isUSB3() const
{
    return (m_boardInfo.Type & IS_INTERFACE_MASK) == IS_INTERFACE_TYPE_USB3;
}

bool Camera::isETH() const
{
    return (m_boardInfo.Type & IS_INTERFACE_MASK) == IS_INTERFACE_TYPE_ETH;
}

bool Camera::isPMC() const
{
    return (m_boardInfo.Type & IS_INTERFACE_MASK) == IS_INTERFACE_TYPE_PMC;
}

bool Camera::isPEAK() const
{
    return (m_boardInfo.Type & IS_INTERFACE_MASK) == IS_INTERFACE_TYPE_IDS_PEAK;
}

bool Camera::isGEV() const
{
    return m_boardInfo.Type == IS_BOARD_TYPE_UEYE_GEV;
}

bool Camera::isU3V() const
{
    return m_boardInfo.Type == IS_BOARD_TYPE_UEYE_U3V;
}

bool Camera::isSE() const
{
    return m_boardInfo.Type == IS_BOARD_TYPE_UEYE_USB_SE;
}

void Camera::updatePixelClockList()
{
    // get Number of Supported PixelClocks
    m_nNumberOfSupportedPixelClocks = 0;
    INT nRet = is_PixelClock(handle(), IS_PIXELCLOCK_CMD_GET_NUMBER,
                         reinterpret_cast<void*>(&m_nNumberOfSupportedPixelClocks),
                         sizeof(m_nNumberOfSupportedPixelClocks));

    if((nRet == IS_SUCCESS) && (m_nNumberOfSupportedPixelClocks > 0))
    {
        m_pixelClockList.resize(static_cast<INT>(m_nNumberOfSupportedPixelClocks));
        if(is_PixelClock(handle(), IS_PIXELCLOCK_CMD_GET_LIST,
                         m_pixelClockList.data(),
                         static_cast<UINT>(m_pixelClockList.size() * sizeof(UINT))) != IS_SUCCESS)
        {
            m_pixelClockList.resize(0);
        }
    } else {
        m_pixelClockList.resize(0);
    }

    pixelclock.update();

    for (int i = 0; i < static_cast<int>(m_nNumberOfSupportedPixelClocks); i++)
    {
        if (pixelclock().toUInt() == static_cast<unsigned int>(m_pixelClockList[i]))
        {
            m_nPixelclockListIndex = i;
        }
    }
    emit pixelclockListChanged(m_pixelClockList, m_nPixelclockListIndex);
}

double Camera::GetApiBandWidth() const
{
    auto bandwidth = is_GetUsedBandwidth(handle());
    return static_cast<double>(bandwidth);
}

INT Camera::loadImage(const QString& filePath) {

    IMAGE_FILE_PARAMS ImageFileParams;
    auto filePathW = filePath.toStdWString();

    ImageFileParams.pwchFileName = const_cast<wchar_t*>(filePathW.c_str());
    ImageFileParams.pnImageID = nullptr;
    ImageFileParams.ppcImageMem = nullptr;
    ImageFileParams.nQuality = 0;

    int nRet = is_ImageFile(getCameraHandle(), IS_IMAGE_FILE_CMD_LOAD, reinterpret_cast<void*>(&ImageFileParams), sizeof(ImageFileParams));
    if (nRet != IS_SUCCESS)
    {
        return nRet;
    }

    processCurrentImageInMemory();

    return IS_SUCCESS;
}

void Camera::processCurrentImageInMemory()
{
    int dummy = 0;
    char *pBuffer = nullptr, *pMem = nullptr;
    is_GetActSeqBuf (getCameraHandle(), &dummy, &pMem, &pBuffer);

    try
    {
        auto buffer = ImageBufferPtr(
                    new helper::LockUnlockSeqBuffer(getCameraHandle(), getImage(pBuffer)));
        if (buffer->OwnsLock())
        {
            frameReceived(buffer);
        }
    }
    catch(const std::exception& e)
    {
        qWarning() << "processCurrentImageInMemory: " << e.what();
    }
}

void Camera::resetErrorCounter()
{
    m_pEvDevice->resetCntFromEvent(IS_SET_EVENT_CAPTURE_STATUS);
    m_pEvDevice->resetCntFromEvent(IS_SET_EVENT_DEVICE_RECONNECTED);
}

bool Camera::isTriggered() const
{
    return m_triggerMode.value().toUInt() != IS_SET_TRIGGER_OFF;
}

void Camera::setTriggerMode(std::uint32_t mode)
{
    m_triggerMode.update();
    auto cur_mode = m_triggerMode.value().toUInt();

    if(cur_mode == mode)
    {
        return;
    }

    emit triggerModeChanged();

    m_setTriggerMode = mode;

    if(isLive() && mode != IS_SET_TRIGGER_OFF)
    {
        stopVideo();
        captureVideo(true);
    }
}

bool Camera::isRollingShutterMode() const
{
    INT caps = 0;
    is_DeviceFeature(m_hCamera, IS_DEVICE_FEATURE_CMD_GET_SUPPORTED_FEATURES, &caps, sizeof(caps));

    if (caps & (IS_DEVICE_FEATURE_CAP_SHUTTER_MODE_ROLLING | IS_DEVICE_FEATURE_CAP_SHUTTER_MODE_GLOBAL))
    {
        INT nShutterMode = 0;
        auto nret = is_DeviceFeature(m_hCamera, IS_DEVICE_FEATURE_CMD_GET_SHUTTER_MODE, &nShutterMode, sizeof(nShutterMode));
        if(nret == IS_SUCCESS)
        {
            return nShutterMode == IS_DEVICE_FEATURE_CAP_SHUTTER_MODE_ROLLING ||
                   nShutterMode == IS_DEVICE_FEATURE_CAP_SHUTTER_MODE_ROLLING_GLOBAL_START;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return !qvariant_cast<SENSORINFO>(sensorInfo()).bGlobShutter;
    }
}

#include "moc_camera.cpp"
