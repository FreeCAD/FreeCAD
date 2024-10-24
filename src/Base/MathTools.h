#ifndef APP_MATHTOOLS_H
#define APP_MATHTOOLS_H

#include <cmath>

namespace Base {

template<class T>
inline T clamp(T num, T lower, T upper)
{
    return std::max<T>(std::min<T>(upper, num), lower);
}

template<class T>
inline T sgn(T t)
{
    if (t == 0) {
        return T(0);
    }

    return (t > 0) ? T(1) : T(-1);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template<class T>
inline T toRadians(T d)
{
    return static_cast<T>((d * M_PI) / 180.0);
}

template<class T>
inline T toDegrees(T r)
{
    return static_cast<T>((r / M_PI) * 180.0);
}

template<class T>
inline T fmod(T numerator, T denominator)
{
    T modulo = std::fmod(numerator, denominator);
    return (modulo >= T(0)) ? modulo : modulo + denominator;
}
}

#endif