/***************************************************************************
 *   Copyright (c) 2019 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

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

///returns an angle in [0..TURN) range
inline DualNumber positiveAngle(DualNumber ang){
    return ang - ::floor((ang.re + 1e-12) / TURN) * TURN;
}

///returns an angle in (-PI..PI] range
inline DualNumber signedAngle(DualNumber ang){
    return ang - ::floor(0.5 + (ang.re - 1e-12) / TURN) * TURN;
}

} //namespace

#endif
