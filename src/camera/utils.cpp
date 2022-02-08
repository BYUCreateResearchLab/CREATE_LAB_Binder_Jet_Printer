#include "utils.h"
#include <ueye.h>
#include <QStandardPaths>


int getBitsPerPixel(int colormode) {
    switch (colormode)
    {

    default:
    case IS_CM_SENSOR_RAW8:
    case IS_CM_MONO8:
        return 8;

    case IS_CM_MONO10:
        return 10;
    case IS_CM_MONO12:
        return 12;

    case IS_CM_SENSOR_RAW10:
    case IS_CM_SENSOR_RAW12:
    case IS_CM_SENSOR_RAW16:
    case IS_CM_MONO16:
    case IS_CM_BGR5_PACKED:
    case IS_CM_BGR565_PACKED:
    case IS_CM_UYVY_PACKED:
    case IS_CM_UYVY_MONO_PACKED:
    case IS_CM_UYVY_BAYER_PACKED:
    case IS_CM_CBYCRY_PACKED:
        return 16;

    case IS_CM_RGB8_PLANAR:
    case IS_CM_RGB8_PACKED:
    case IS_CM_BGR8_PACKED:
        return 24;

    case IS_CM_RGBA8_PACKED:
    case IS_CM_RGBY8_PACKED:
    case IS_CM_RGB10_PACKED:
    case IS_CM_BGRA8_PACKED:
    case IS_CM_BGRY8_PACKED:
    case IS_CM_BGR10_PACKED:
        return 32;

    case IS_CM_RGB10_UNPACKED:
    case IS_CM_RGB12_UNPACKED:
    case IS_CM_BGR10_UNPACKED:
    case IS_CM_BGR12_UNPACKED:
        return 48;

    case IS_CM_RGBA12_UNPACKED:
    case IS_CM_BGRA12_UNPACKED:
        return 64;
    }
}

int GetMaxPixelValue(int colormode)
{
    switch (colormode &~ IS_CM_ORDER_RGB)
    {
    case IS_CM_RGB8_PACKED:
    case IS_CM_BGRA8_PACKED:
    case IS_CM_BGR8_PACKED:
        return 255;
    case IS_CM_BGR565_PACKED:
        return 0x3f;
    case IS_CM_BGR5_PACKED:
        return 0x1f;
    case IS_CM_UYVY_PACKED:
        return 255;
    case IS_CM_MONO8:
    case IS_CM_SENSOR_RAW8:
        return 255;
    case IS_CM_MONO10:
    case (IS_CM_MONO10 | IS_CM_PREFER_PACKED_SOURCE_FORMAT):
    case IS_CM_SENSOR_RAW10:
    case (IS_CM_SENSOR_RAW10 | IS_CM_PREFER_PACKED_SOURCE_FORMAT):
        return 1023;
    case IS_CM_MONO12:
    case IS_CM_SENSOR_RAW12:
        return 4095;
    case IS_CM_MONO16:
    case IS_CM_SENSOR_RAW16:
        return 65535;
    case IS_CM_BGRY8_PACKED:
        return 255;
    case IS_CM_BGR10_UNPACKED:
    case (IS_CM_BGR10_UNPACKED | IS_CM_PREFER_PACKED_SOURCE_FORMAT):
    case IS_CM_BGR10_PACKED:
        return 1023;
    case IS_CM_BGR12_UNPACKED:
    case IS_CM_BGRA12_UNPACKED:
        return 4095;
    default:
        break;
    }
    return 8;
}

QImage::Format getQtFormat(int colormode) {
    switch (colormode)
    {
    case IS_CM_MONO8:
    case IS_CM_MONO10:
    case IS_CM_MONO12:
    case IS_CM_MONO16:
    case IS_CM_SENSOR_RAW8:
    case IS_CM_SENSOR_RAW10:
    case IS_CM_SENSOR_RAW12:
    case IS_CM_SENSOR_RAW16:
        return QImage::Format_Indexed8;

    /*
     * some of those don't have to be 888...
     */
    case IS_CM_BGRY8_PACKED:
    case IS_CM_BGR10_PACKED:
    case IS_CM_BGR10_UNPACKED:
    case IS_CM_BGR12_UNPACKED:
    case IS_CM_BGRA12_UNPACKED:
    case IS_CM_RGB8_PLANAR:
    case IS_CM_RGB8_PACKED:
    case IS_CM_RGBY8_PACKED:
    case IS_CM_RGB10_PACKED:
    case IS_CM_RGB10_UNPACKED:
    case IS_CM_RGB12_UNPACKED:
    case IS_CM_RGBA12_UNPACKED:
    case IS_CM_UYVY_PACKED:
    case IS_CM_UYVY_MONO_PACKED:
    case IS_CM_UYVY_BAYER_PACKED:
    case IS_CM_CBYCRY_PACKED:
        return QImage::Format_RGB888;
    case IS_CM_BGR565_PACKED:
        return QImage::Format_RGB16;
    case IS_CM_BGR5_PACKED:
        return QImage::Format_RGB555;
    case IS_CM_RGBA8_PACKED:
    case IS_CM_BGRA8_PACKED:
        return QImage::Format_RGB32;

    case IS_CM_BGR8_PACKED:
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        return QImage::Format_RGB888;
#else
        return QImage::Format_BGR888;
#endif


    default:
        return QImage::Format::Format_Invalid;
    }
}

int maxBandwidth(std::uint32_t hCam)
{
    /* Get the settings from the cam */
    auto nBusSpeed = is_GetBusSpeed(static_cast<HIDS>(hCam));
    auto nDeviceType = is_GetCameraType(static_cast<HIDS>(hCam));
    auto nInterfaceType = nDeviceType & 0xF0;

    auto nMaxBandwidth = 0;

    /* Set the upper boarders for the bandwidth */
    if (nInterfaceType == IS_INTERFACE_TYPE_USB3)
    {
        /* Patch USB3 camera at USB2 port */
        if (nBusSpeed == IS_USB_20)
        {
            nMaxBandwidth = 38;
        }
        else
        {
            nMaxBandwidth = 400;
        }
    }
    else if (nInterfaceType == IS_INTERFACE_TYPE_USB)
    {
        nMaxBandwidth = 38;
    }
    else if (nInterfaceType == IS_INTERFACE_TYPE_ETH)
    {
        if (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_HE)
        {
            nMaxBandwidth = 105;
        }
        else if ((nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_CP_R2) ||
            (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_FA) ||
            (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_SE_R4))
        {
            nMaxBandwidth = 110;
        }
        else if ((nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_SE) || (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_CP) ||
                 (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_LE) || (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_REP) ||
                 (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_TE) || (nDeviceType == IS_CAMERA_TYPE_UEYE_ETH_LEET))
        {
            nMaxBandwidth = 70;
        }

        /* Patch low speed interface bandwith */
        if (nBusSpeed == IS_ETHERNET_100)
        {
            nMaxBandwidth = 10;
        }
    }
    else if(nDeviceType == IS_BOARD_TYPE_UEYE_U3V || nDeviceType == IS_BOARD_TYPE_UEYE_GEV)
    {
        nMaxBandwidth = nBusSpeed / 8;
    }
    return nMaxBandwidth;
}

QDateTime fromuEyeTime(UEYETIME ueye_time)
{
    tm ueyetm{};
    time_t now = time(nullptr);
    ueyetm = *localtime(&now); //also sets dst, may be wrong a few times around change of that

    ueyetm.tm_yday = 0;
    ueyetm.tm_wday = 0;
    ueyetm.tm_mday = ueye_time.wDay;
    ueyetm.tm_mon = ueye_time.wMonth - 1;
    ueyetm.tm_year = ueye_time.wYear - 1900;
    ueyetm.tm_hour = ueye_time.wHour;
    ueyetm.tm_sec = ueye_time.wSecond;
    ueyetm.tm_min = ueye_time.wMinute;

    return QDateTime::fromTime_t(mktime(&ueyetm));
}

QString userPictureDirectory()
{
    auto dir = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if(dir.empty())
    {
        return {};
    }
    else
    {
        return dir[0];
    }
}

QString userDirectory()
{
    auto dir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    if(dir.empty())
    {
        return {};
    }
    else
    {
        return dir[0];
    }
}

