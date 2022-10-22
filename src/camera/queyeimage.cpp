#include "queyeimage.h"
#include "utils.h"
#include <QDebug>
#include <utility>

using namespace uEyeAssist;
using namespace ImageFormat;


QuEyeImage::QuEyeImage(const uchar* pcMem, int nWidth, int nHeight, int nColorMode)
        : QImage(nWidth, nHeight, getQtFormat(nColorMode))
        , m_srcChannels(0)
        , m_srcBitsChannel(0)
        , m_srcBitsTotal(0)
        , m_destChannels(0)
        , m_destBitsChannel(0)
        , m_destBitsTotal(0)
{
    bool isMono;
    switch (nColorMode)
    {
    case IS_CM_MONO16:
    case IS_CM_MONO12:
    case IS_CM_MONO10:
    case IS_CM_MONO8: {
        isMono = true;
        break;
    }
    default: {
        isMono = false;
    }
    }

    loadConversionInfo(nColorMode, isMono);
    // todo: those multiplications are dangerous
    switch (nColorMode)
    {
    case IS_CM_MONO8:
        memcpy(scanLine(0),
               pcMem,
               static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()) * ((m_destBitsTotal + 7) / 8));
        break;

    case IS_CM_MONO10:
        convertToMono8<PIX_MONO10>(scanLine(0), pcMem);
        break;

    case IS_CM_MONO12:
        convertToMono8<PIX_MONO12>(scanLine(0), pcMem);
        break;

    case IS_CM_MONO16:
        convertToMono8<PIX_MONO16>(scanLine(0), pcMem);
        break;

    case IS_CM_BGR565_PACKED:
        memcpy(scanLine(0),
               pcMem,
               static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()) * ((m_destBitsTotal + 7) / 8));
        break;

    case IS_CM_BGR5_PACKED:
        memcpy(scanLine(0),
               pcMem,
               static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()) * ((m_destBitsTotal + 7) / 8));
        break;

    case IS_CM_BGR8_PACKED:
        memcpy(scanLine(0),
               pcMem,
               static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()) * ((m_destBitsTotal + 7) / 8));
        break;

    case IS_CM_BGRY8_PACKED:
        convertToRgb888<PIX_BGRY8_PACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_BGRA8_PACKED:
        memcpy(scanLine(0),
               pcMem,
               static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()) * ((m_destBitsTotal + 7) / 8));
        break;

    case IS_CM_BGR10_PACKED:
        convertToRgb888<PIX_BGR10_PACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_BGR10_UNPACKED:
        convertToRgb888<PIX_BGR10_UNPACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_BGR12_UNPACKED:
        convertToRgb888<PIX_BGR12_UNPACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_BGRA12_UNPACKED:
        convertToRgb888<PIX_BGRA12_UNPACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_RGB8_PACKED:
        memcpy(scanLine(0),
               pcMem,
               static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()) * ((m_destBitsTotal + 7) / 8));
        break;

    case IS_CM_RGBY8_PACKED:
        convertToRgb888<PIX_RGBY8_PACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_RGBA8_PACKED:
        memcpy(scanLine(0),
               pcMem,
               static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()) * ((m_destBitsTotal + 7) / 8));
        break;

    case IS_CM_RGB10_PACKED:
        convertToRgb888<PIX_RGB10_PACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_RGB10_UNPACKED:
        convertToRgb888<PIX_RGB10_UNPACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_RGB12_UNPACKED:
        convertToRgb888<PIX_RGB12_UNPACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_RGBA12_UNPACKED:
        convertToRgb888<PIX_RGBA12_UNPACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_RGB8_PLANAR:
        convertPlanarToRgb888(scanLine(0), pcMem);
        break;

    case IS_CM_SENSOR_RAW8:
        memcpy(scanLine(0), pcMem, static_cast<std::size_t>(width()) * static_cast<std::size_t>(height()));
        break;

    case IS_CM_SENSOR_RAW10:
        if (isMono)
        {
            convertToMono8<PIX_MONO10RAW>(scanLine(0), pcMem);
        }
        else
        {
            convertRawRgbToMono8<PIX_RGB10RAW>(scanLine(0), pcMem);
        }
        break;

    case IS_CM_SENSOR_RAW12:
        if (isMono)
        {
            convertToMono8<PIX_MONO12RAW>(scanLine(0), pcMem);
        }
        else
        {
            convertRawRgbToMono8<PIX_RGB12RAW>(scanLine(0), pcMem);
        }
        break;

    case IS_CM_SENSOR_RAW16:
        if (isMono)
        {
            convertToMono8<PIX_MONO16RAW>(scanLine(0), pcMem);
        }
        else
        {
            convertRawRgbToMono8<PIX_RGB16RAW>(scanLine(0), pcMem);
        }
        break;

    case IS_CM_UYVY_PACKED:
        convertYuv442ToRgb888<PIX_UYVY_PACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_UYVY_MONO_PACKED:
        convertYuv442ToRgb888<PIX_UYVY_MONO_PACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_UYVY_BAYER_PACKED:
        convertYuv442ToRgb888<PIX_UYVY_BAYER_PACKED>(scanLine(0), pcMem);
        break;

    case IS_CM_CBYCRY_PACKED:
        convertYCbCrToRgb888<PIX_CBYCRY_PACKED>(scanLine(0), pcMem);
        break;

    default:
        break;
    }

    switch (nColorMode)
    {
    case IS_CM_SENSOR_RAW8:
    case IS_CM_SENSOR_RAW10:
    case IS_CM_SENSOR_RAW12:
    case IS_CM_SENSOR_RAW16:
    case IS_CM_MONO8:
    case IS_CM_MONO10:
    case IS_CM_MONO12:
    case IS_CM_MONO16: {
        for (int i = 0; i < 256; i++)
            m_table.push_back(qRgb(i, i, i));
        setColorCount(256);
        setColorTable(m_table);
        break;
    }

    default:
        break;
    }
}

void QuEyeImage::loadConversionInfo(int nColorMode, bool isMono)
{
    switch (nColorMode)
    {
    // RAW
    case IS_CM_SENSOR_RAW8:
        m_srcChannels = isMono ? 1 : 2;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = isMono ? 1 : 2;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_SENSOR_RAW10:
        m_srcChannels = isMono ? 1 : 2;
        m_srcBitsChannel = 10;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = isMono ? 1 : 2;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_SENSOR_RAW12:
        m_srcChannels = isMono ? 1 : 2;
        m_srcBitsChannel = 12;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = isMono ? 1 : 2;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_SENSOR_RAW16:
        m_srcChannels = isMono ? 1 : 2;
        m_srcBitsChannel = 16;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = isMono ? 1 : 2;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    // MONO
    case IS_CM_MONO8:
        m_srcChannels = 1;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 1;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_MONO10:
        m_srcChannels = 1;
        m_srcBitsChannel = 10;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 1;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_MONO12:
        m_srcChannels = 1;
        m_srcBitsChannel = 12;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 1;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_MONO16:
        m_srcChannels = 1;
        m_srcBitsChannel = 16;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 1;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;
    // RGB
    case IS_CM_RGB8_PACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_RGBY8_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_RGBA8_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 4;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_RGB10_PACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 10;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_RGB10_UNPACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 10;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_RGB12_UNPACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 12;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_RGBA12_UNPACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 12;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    // BGR
    case IS_CM_BGR5_PACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 5;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 5;
        m_destBitsTotal = 16; // dont apply formula on this format
        break;

    case IS_CM_BGR565_PACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 5;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 5;
        m_destBitsTotal = 16; // don't apply formula on this format
        break;

    case IS_CM_BGR8_PACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_BGRY8_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_BGRA8_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 4;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_BGR10_PACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 10;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_BGR10_UNPACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 10;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_BGR12_UNPACKED:
        m_srcChannels = 3;
        m_srcBitsChannel = 12;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_BGRA12_UNPACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 12;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    // PLANAR
    case IS_CM_RGB8_PLANAR:
        m_srcChannels = 3;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 3;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    // YUV
    case IS_CM_UYVY_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 6;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_UYVY_MONO_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 6;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    case IS_CM_UYVY_BAYER_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 6;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    // CBYCRY
    case IS_CM_CBYCRY_PACKED:
        m_srcChannels = 4;
        m_srcBitsChannel = 8;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 6;
        m_destBitsChannel = 8;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;

    default:
        m_srcChannels = 0;
        m_srcBitsChannel = 0;
        m_srcBitsTotal = m_srcChannels * alignBitCount<uint8_t>(m_srcBitsChannel, 8);
        m_destChannels = 0;
        m_destBitsChannel = 0;
        m_destBitsTotal = m_destChannels * alignBitCount<uint8_t>(m_destBitsChannel, 8);
        break;
    }
}

void QuEyeImage::convertYuv444ToRgb888(uint8_t y, uint8_t u, uint8_t v, uint8_t* r, uint8_t* g, uint8_t* b)
{
    auto c = static_cast<std::int16_t>(y - 16);
    auto d = static_cast<std::int16_t>(u - 128);
    auto e = static_cast<std::int16_t>(v - 128);

    *r = clamp<uint8_t>(static_cast<uchar>((298 * c + 409 * e + 128) >> 8), 0, 255);
    *g = clamp<uint8_t>(static_cast<uchar>((298 * c - 100 * d - 208 * e + 128) >> 8), 0, 255);
    *b = clamp<uint8_t>(static_cast<uchar>((298 * c + 516 * d + 128) >> 8), 0, 255);
}

void QuEyeImage::convertYCbCrToRgb888(uint8_t y, uint8_t cb, uint8_t cr, uint8_t* r, uint8_t* g, uint8_t* b)
{
    *r = clamp<uint8_t>(static_cast<uchar>(y + 1.402 * (cr - 128)), 0, 255);
    *g = clamp<uint8_t>(static_cast<uchar>(y - 0.34414 * (cb - 128) - 0.71414 * (cr - 128)), 0, 255);
    *b = clamp<uint8_t>(static_cast<uchar>(y + 1.772 * (cb - 128)), 0, 255);
}

void QuEyeImage::convertPlanarToRgb888(uchar* pcDest, const uchar* pcSrc)
{
    uint32_t channelSize = static_cast<uint32_t>(width()) * static_cast<uint32_t>(height());
    for (size_t i = 0; i < channelSize; i++)
    {
        pcDest[m_destChannels * i + 0] = static_cast<uchar>(pcSrc[i] >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
        pcDest[m_destChannels * i + 1] =
            static_cast<uchar>(pcSrc[i + channelSize] >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
        pcDest[m_destChannels * i + 2] =
            static_cast<uchar>(pcSrc[i + channelSize * 2] >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
    }
}

QuEyeImage::QuEyeImage(QImage&& image) : QImage(std::move(image)) {}
QuEyeImage& QuEyeImage::operator=(QImage&& image)
{
    qSwap(*static_cast<QImage*>(this), image);
    return *this;
};

#include "moc_queyeimage.cpp"
