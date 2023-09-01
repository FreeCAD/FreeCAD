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

#ifndef FREECAD_BASE_DUAL_QUATERNION_H
#define FREECAD_BASE_DUAL_QUATERNION_H

#include "DualNumber.h"
#include <FCGlobal.h>


namespace Base {

/**
 * @brief The DualQuat class represents a dual quaternion, as a quaternion of
 * dual number components. Dual quaternions are useful for placement
 * interpolation, see pow method.
 *
 * Rotation is stored as non-dual part of DualQ. Translation is encoded into
 * dual part of DualQuat:
 * DualQuat.dual() = 0.5 * t * r,
 * where t is quaternion with x,y,z of translation and w of 0, and r is the
 * rotation quaternion.
 */
class BaseExport DualQuat {
public:
    DualNumber x;
    DualNumber y;
    DualNumber z;
    DualNumber w;
public:
    ///default constructor: init with zeros
    DualQuat() = default;
    DualQuat(DualNumber x, DualNumber y, DualNumber z, DualNumber w)
        : x(x), y(y), z(z), w(w) {}
    DualQuat(double x,double y,double z,double w,double dx,double dy,double dz,double dw)
        : x(x, dx), y(y, dy), z(z, dz), w(w, dw) {}
    DualQuat(double x,double y,double z,double w)
        : x(x), y(y), z(z), w(w) {}

    ///Builds a dual quaternion from real and dual parts provided as pure real quaternions
    DualQuat(DualQuat re, DualQuat du);

    ///returns dual quaternion for identity placement
    static DualQuat identity() {return {0.0, 0.0, 0.0, 1.0};}

    ///return a copy with dual part zeroed out
    DualQuat real() const {return {x.re, y.re, z.re, w.re};}

    ///return a real-only quaternion made from dual part of this quaternion.
    DualQuat dual() const {return {x.du, y.du, z.du, w.du};}

    ///conjugate
    DualQuat conj() const {return {-x, -y, -z, w};}

    ///return vector part (with scalar part zeroed out)
    DualQuat vec() const {return {x,y,z,0.0};}

    ///magnitude of the quaternion
    double length() const {return sqrt(x.re*x.re + y.re*y.re + z.re*z.re + w.re*w.re);}

    ///angle of rotation represented by this quaternion, in radians
    double theta() const {return 2.0 * atan2(vec().length(), w.re);}

    ///dot product between real (rotation) parts of two dual quaternions (to determine if one of them should be negated for shortest interpolation)
    static double dot(DualQuat a, DualQuat b);

    ///ScLERP. t=0.0 returns identity, t=1.0 returns this. t can also be outside of 0..1 bounds.
    DualQuat pow(double t, bool shorten = true) const;

    DualQuat operator-() const {return {-x, -y, -z, -w};}

    //DEBUG
    //void print() const {
    //    Console().Log("%f, %f, %f, %f; %f, %f, %f, %f", x.re,y.re,z.re,w.re, x.du,y.du,z.du, w.du);
    //}
};

BaseExport DualQuat operator+(DualQuat a, DualQuat b);
BaseExport DualQuat operator-(DualQuat a, DualQuat b);
BaseExport DualQuat operator*(DualQuat a, DualQuat b);

BaseExport DualQuat operator*(DualQuat a, double b);
BaseExport DualQuat operator*(double a, DualQuat b);
BaseExport DualQuat operator*(DualQuat a, DualNumber b);
BaseExport DualQuat operator*(DualNumber a, DualQuat b);


} //namespace

#endif
