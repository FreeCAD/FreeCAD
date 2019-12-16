
#ifndef FREECAD_CONSTRAINTSOLVER_DUALMATH_H
#define FREECAD_CONSTRAINTSOLVER_DUALMATH_H

#include "Base/DualNumber.h"
#include "cmath"

namespace FCS {

using DualNumber = Base::DualNumber;

inline double sq(double x){
    return x*x;
}

inline DualNumber sq(DualNumber x){
    return DualNumber(sq(x.re), 2 * x.re * x.du);
}

using ::sqrt;//duanumber's sqrt tends to override the standard one, and causes errors

inline DualNumber sqrt(DualNumber x){
    double re = ::sqrt(x.re);
    return DualNumber(re, x.du * 0.5 / re);
}

inline DualNumber sin(DualNumber ang) {
    return DualNumber(::sin(ang.re), ang.du * ::cos(ang.re));
}

inline DualNumber cos(DualNumber ang) {
    return DualNumber(::cos(ang.re), -ang.du * ::sin(ang.re));
}

inline DualNumber sinh(DualNumber ang) {
    return DualNumber(::sinh(ang.re), ang.du * ::cosh(ang.re));
}

inline DualNumber cosh(DualNumber ang) {
    return DualNumber(::cosh(ang.re), ang.du * ::sinh(ang.re));
}

inline DualNumber atan2(DualNumber y, DualNumber x) {
    double re = ::atan2(y.re, x.re);
    double du = (x.du * -y.re + y.du * x.re)/(sq(x.re) + sq(y.re));
    return DualNumber(re, du);
}

///atan2 assuming x^2+y^2 == 1 (slightly faster)
inline DualNumber atan2n(DualNumber y, DualNumber x) {
    double re = ::atan2(y.re, x.re);
    double du = (x.du * -y.re + y.du * x.re);
    return DualNumber(re, du);
}

inline DualNumber exp(DualNumber a){
    double ret = ::exp(a.re);
    return DualNumber(ret, a.du * ret);
}

inline DualNumber ln(DualNumber a){
    return DualNumber(::log(a.re), a.du / a.re);
}

///2*pi
#define TURN 6.28318530717958647692528676655900576839434

} //namespace

#endif
