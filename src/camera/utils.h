#ifndef UTILS_H
#define UTILS_H

#include <QImage>
#include <ueye.h>
#include <QDateTime>

#if __cplusplus >= 201703L || _MSVC_LANG >=201703L // MSVC still reports c++98...
#    define NO_DISCARD [[nodiscard]]
#    define MAYBE_UNUSED  [[maybe_unused]]
#else
#    ifdef _MSC_VER
#        define NO_DISCARD _Check_return_
#        define MAYBE_UNUSED
#    elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#        define NO_DISCARD __attribute__((warn_unused_result))
#        define MAYBE_UNUSED __attribute__((unused))
#    else
#        error unknown compiler
#    endif
#endif

#define DEMO_SAVESTATE_VERSION 2

int GetMaxPixelValue(int colormode);
int getBitsPerPixel(int colormode);
QImage::Format getQtFormat(int colormode);

//as we have to support "old" Qt version, we can't just use qOverload,
//define template here
//source: https://stackoverflow.com/questions/16794695/connecting-overloaded-signals-and-slots-in-qt-5

template<typename... Args> struct SELECT {
    template<typename C, typename R>
    static constexpr auto OVERLOAD_OF( R (C::*pmf)(Args...) ) -> decltype(pmf) {
        return pmf;
    }
};

int maxBandwidth(std::uint32_t hCam);

QDateTime fromuEyeTime(UEYETIME ueye_time);
QString userPictureDirectory();
QString userDirectory();

#endif // UTILS_H
