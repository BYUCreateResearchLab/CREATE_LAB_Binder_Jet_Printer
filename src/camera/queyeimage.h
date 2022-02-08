#ifndef QUEYEIMAGE_H
#define QUEYEIMAGE_H

#include <algorithm>
#include <cstdint>

#include <QImage>
#include <QMetaType>
#include <QSharedPointer>
#include <QtGlobal>

#include "utils.h"
#include <ueye.h>

namespace uEyeAssist
{
namespace ImageFormat
{

#pragma pack(push, 1)

// mono
struct PIX_MONO8
{
    std::uint8_t y;
};

struct PIX_MONO10
{
    std::uint16_t y;
};

typedef PIX_MONO10 PIX_MONO12;
typedef PIX_MONO10 PIX_MONO16;

// raw
typedef PIX_MONO8 PIX_MONO8RAW;
typedef PIX_MONO10 PIX_MONO10RAW;
typedef PIX_MONO12 PIX_MONO12RAW;
typedef PIX_MONO16 PIX_MONO16RAW;

struct PIX_RGB8RAW
{
    std::uint8_t c1;
    std::uint8_t c2;
};

struct PIX_RGB10RAW
{
    std::uint16_t c1;
    std::uint16_t c2;
};

typedef PIX_RGB10RAW PIX_RGB12RAW;
typedef PIX_RGB10RAW PIX_RGB16RAW;

// rgb
struct PIX_RGB8_PACKED
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

struct PIX_RGBY8_PACKED
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t y;
};

struct PIX_RGBA8_PACKED
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

struct PIX_RGB10_PACKED
{
    std::uint32_t r : 10;
    std::uint32_t g : 10;
    std::uint32_t b : 10;
    std::uint32_t empty : 2;
};

struct PIX_RGB10_UNPACKED
{
    std::uint16_t r;
    std::uint16_t g;
    std::uint16_t b;
};

struct PIX_RGB12_UNPACKED
{
    std::uint16_t r;
    std::uint16_t g;
    std::uint16_t b;
};

struct PIX_RGBA12_UNPACKED
{
    std::uint16_t r;
    std::uint16_t g;
    std::uint16_t b;
    std::uint16_t a;
};

// bgr
struct PIX_BGR5_PACKED
{
    std::uint16_t b : 5;
    std::uint16_t g : 5;
    std::uint16_t r : 5;
    std::uint16_t empty : 1;
};

struct PIX_BGR565_PACKED
{
    std::uint16_t b : 5;
    std::uint16_t g : 6;
    std::uint16_t r : 5;
};

struct PIX_BGR8_PACKED
{
    std::uint8_t b;
    std::uint8_t g;
    std::uint8_t r;
};

struct PIX_BGRY8_PACKED
{
    std::uint8_t b;
    std::uint8_t g;
    std::uint8_t r;
    std::uint8_t y;
};

struct PIX_BGRA8_PACKED
{
    std::uint8_t b;
    std::uint8_t g;
    std::uint8_t r;
    std::uint8_t a;
};

struct PIX_BGR10_PACKED
{
    std::uint32_t b : 10;
    std::uint32_t g : 10;
    std::uint32_t r : 10;
    std::uint32_t empty : 2;
};

struct PIX_BGR10_UNPACKED
{
    std::uint16_t b;
    std::uint16_t g;
    std::uint16_t r;
};

struct PIX_BGR12_UNPACKED
{
    std::uint16_t b;
    std::uint16_t g;
    std::uint16_t r;
};

struct PIX_BGRA12_UNPACKED
{
    std::uint16_t b;
    std::uint16_t g;
    std::uint16_t r;
    std::uint16_t a;
};

// yuv
struct PIX_UYVY_PACKED
{
    std::uint8_t u;
    std::uint8_t y1;
    std::uint8_t v;
    std::uint8_t y2;
};

typedef PIX_UYVY_PACKED PIX_UYVY_MONO_PACKED;
typedef PIX_UYVY_PACKED PIX_UYVY_BAYER_PACKED;

struct PIX_CBYCRY_PACKED
{
    std::uint8_t cb;
    std::uint8_t y1;
    std::uint8_t cr;
    std::uint8_t y2;
};

#pragma pack(pop)

} // namespace ImageFormat

class QuEyeImage : public QImage
{
private:
    // image info for source data
    std::uint8_t m_srcChannels{};
    std::uint8_t m_srcBitsChannel{};
    std::uint8_t m_srcBitsTotal{};

    // image info for destination data
    std::uint8_t m_destChannels{};
    std::uint8_t m_destBitsChannel{};
    std::uint8_t m_destBitsTotal{};
    QVector<QRgb> m_table;

public:
    QuEyeImage(const uchar* pcMem, int nWidth, int nHeight, int nColorMode);

    QuEyeImage() = default;

    QuEyeImage& operator=(const QuEyeImage& image) = default;
    QuEyeImage& operator=(QuEyeImage&& image) = default;

    QuEyeImage& operator=(QImage&& image);

    explicit QuEyeImage(QImage image) = delete;
    explicit QuEyeImage(QImage&& image);

private:
    void loadConversionInfo(int nColorMode, bool isMono);

    template <typename pixel_t>
    void convertToRgb888(uchar* pcDest, const uchar* pcSrc)
    {
        const auto* temp = reinterpret_cast<const pixel_t*>(pcSrc);
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(width()) * static_cast<std::uint32_t>(height()); i++)
        {
            pcDest[m_destChannels * i + 0] =
                static_cast<uchar>(temp[i].r >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
            pcDest[m_destChannels * i + 1] =
                static_cast<uchar>(temp[i].g >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
            pcDest[m_destChannels * i + 2] =
                static_cast<uchar>(temp[i].b >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
        }
    }

    template <typename pixel_t>
    void convertToMono8(uchar* pcDest, const uchar* pcSrc)
    {
        const auto* temp = reinterpret_cast<const pixel_t*>(pcSrc);
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(width()) * static_cast<std::uint32_t>(height()); i++)
        {
            pcDest[i] = static_cast<char>(temp[i].y >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
        }
    }

    template <typename pixel_t>
    void convertRawRgbToMono8(uchar* pcDest, const uchar* pcSrc)
    {
        const auto* temp = reinterpret_cast<const pixel_t*>(pcSrc);
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(width() * height() / m_destChannels); i++)
        {
            pcDest[m_destChannels * i + 0] =
                static_cast<uchar>(temp[i].c1 >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
            pcDest[m_destChannels * i + 1] =
                static_cast<uchar>(temp[i].c2 >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF);
        }
    }

    template <typename pixel_t>
    void convertYuv442ToRgb888(uchar* pcDest, const uchar* pcSrc)
    {
        const auto* temp = reinterpret_cast<const pixel_t*>(pcSrc);
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(width()) * static_cast<std::uint32_t>(height()); i++)
        {
            std::uint8_t y1 = static_cast<std::uint8_t>(temp[i].y1) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;
            std::uint8_t u = static_cast<std::uint8_t>(temp[i].u) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;
            std::uint8_t y2 = static_cast<std::uint8_t>(temp[i].y2) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;
            std::uint8_t v = static_cast<std::uint8_t>(temp[i].v) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;

            std::uint8_t r1 = 0;
            std::uint8_t g1 = 0;
            std::uint8_t b1 = 0;
            std::uint8_t r2 = 0;
            std::uint8_t g2 = 0;
            std::uint8_t b2 = 0;

            convertYuv444ToRgb888(y1, u, v, &r1, &g1, &b1);
            convertYuv444ToRgb888(y2, u, v, &r2, &g2, &b2);

            pcDest[m_destChannels * i + 0] = r1;
            pcDest[m_destChannels * i + 1] = g1;
            pcDest[m_destChannels * i + 2] = b1;
            pcDest[m_destChannels * i + 3] = r2;
            pcDest[m_destChannels * i + 4] = g2;
            pcDest[m_destChannels * i + 5] = b2;
        }
    }

    template <typename pixel_t>
    void convertYCbCrToRgb888(uchar* pcDest, const uchar* pcSrc)
    {
        const auto* temp = reinterpret_cast<const pixel_t*>(pcSrc);
        for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(width()) * static_cast<std::uint32_t>(height()); i++)
        {
            std::uint8_t y1 = static_cast<std::uint8_t>(temp[i].y1) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;
            std::uint8_t cb = static_cast<std::uint8_t>(temp[i].cb) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;
            std::uint8_t y2 = static_cast<std::uint8_t>(temp[i].y2) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;
            std::uint8_t cr = static_cast<std::uint8_t>(temp[i].cr) >> (m_srcBitsChannel - m_destBitsChannel) & 0xFF;

            std::uint8_t r1 = 0;
            std::uint8_t g1 = 0;
            std::uint8_t b1 = 0;
            std::uint8_t r2 = 0;
            std::uint8_t g2 = 0;
            std::uint8_t b2 = 0;

            convertYCbCrToRgb888(y1, cb, cr, &r1, &g1, &b1);
            convertYCbCrToRgb888(y2, cb, cr, &r2, &g2, &b2);

            pcDest[m_destChannels * i + 0] = r1;
            pcDest[m_destChannels * i + 1] = g1;
            pcDest[m_destChannels * i + 2] = b1;
            pcDest[m_destChannels * i + 3] = r2;
            pcDest[m_destChannels * i + 4] = g2;
            pcDest[m_destChannels * i + 5] = b2;
        }
    }

    void convertPlanarToRgb888(uchar* pcDest, const uchar* pcSrc);
    void convertYuv444ToRgb888(std::uint8_t y,
                               std::uint8_t u,
                               std::uint8_t v,
                               std::uint8_t* r,
                               std::uint8_t* g,
                               std::uint8_t* b);
    void convertYCbCrToRgb888(std::uint8_t y,
                              std::uint8_t cb,
                              std::uint8_t cr,
                              std::uint8_t* r,
                              std::uint8_t* g,
                              std::uint8_t* b);

    template <typename type_t>
    NO_DISCARD inline type_t clamp(type_t nValue, type_t nMin, type_t nMax)
    {
        return qMax(nMin, qMin(nMax, nValue));
    }

    template <typename type_t>
    NO_DISCARD inline type_t alignBitCount(type_t bits, type_t alignment)
    {
        return static_cast<type_t>((bits + (alignment - 1)) / alignment) * alignment;
    }
};
} // namespace uEyeAssist

#endif // QUEYEIMAGE_H
