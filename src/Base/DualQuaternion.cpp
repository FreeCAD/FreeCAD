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

#include "PreCompiled.h"

#include <cassert>
#include "DualQuaternion.h"


Base::DualQuat Base::operator+(Base::DualQuat a, Base::DualQuat b)
{
    return {
                a.x + b.x,
                a.y + b.y,
                a.z + b.z,
                a.w + b.w
                };
}

Base::DualQuat Base::operator-(Base::DualQuat a, Base::DualQuat b)
{
    return {
                a.x - b.x,
                a.y - b.y,
                a.z - b.z,
                a.w - b.w
                };
}

Base::DualQuat Base::operator*(Base::DualQuat a, Base::DualQuat b)
{
    return {
                a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
                a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
                };
}

Base::DualQuat Base::operator*(Base::DualQuat a, double b)
{
    return {
                a.x * b,
                a.y * b,
                a.z * b,
                a.w * b
                };
}

Base::DualQuat Base::operator*(double a, Base::DualQuat b)
{
    return {
                b.x * a,
                b.y * a,
                b.z * a,
                b.w * a
                };
}

Base::DualQuat Base::operator*(Base::DualQuat a, Base::DualNumber b)
{
    return {
                a.x * b,
                a.y * b,
                a.z * b,
                a.w * b
                };
}

Base::DualQuat Base::operator*(Base::DualNumber a, Base::DualQuat b)
{
    return {
                b.x * a,
                b.y * a,
                b.z * a,
                b.w * a
                };
}

Base::DualQuat::DualQuat(Base::DualQuat re, Base::DualQuat du)
    : x(re.x.re, du.x.re),
      y(re.y.re, du.y.re),
      z(re.z.re, du.z.re),
      w(re.w.re, du.w.re)
{
    assert(re.dual().length() < 1e-12);
    assert(du.dual().length() < 1e-12);
}

double Base::DualQuat::dot(Base::DualQuat a, Base::DualQuat b)
{
    return  a.x.re * b.x.re +
            a.y.re * b.y.re +
            a.z.re * b.z.re +
            a.w.re * b.w.re   ;
}

Base::DualQuat Base::DualQuat::pow(double t, bool shorten) const
{
    /* implemented after "Dual-Quaternions: From Classical Mechanics to
     * Computer Graphics and Beyond" by Ben Kenwright www.xbdev.net
     * bkenwright@xbdev.net
     * http://www.xbdev.net/misc_demos/demos/dual_quaternions_beyond/paper.pdf
     *
     * There are some differences:
     *
     * * Special handling of no-rotation situation (because normalization
     * multiplier becomes infinite in this situation, breaking the algorithm).
     *
     * * Dual quaternions are implemented as a collection of dual numbers,
     * rather than a collection of two quaternions like it is done in suggested
     * implementation in the paper.
     *
     * * acos replaced with atan2 for improved angle accuracy for small angles
     *
     * */
    double le = this->vec().length();
    if (le < 1e-12) {
        //special case of no rotation. Interpolate position
        return {this->real(), this->dual()*t};
    }

    double normmult = 1.0/le;

    DualQuat self = *this;
    if (shorten){
        if (dot(self, identity()) < -1e-12){ //using negative tolerance instead of zero, for stability in situations the choice is ambiguous (180-degree rotations)
            self = -self;
        }
    }

    //to screw coordinates
    double theta = self.theta();
    double pitch = -2.0 * self.w.du * normmult;
    DualQuat l = self.real().vec() * normmult; //abusing DualQuat to store vectors. Very handy in this case.
    DualQuat m = (self.dual().vec() - pitch/2*cos(theta/2)*l)*normmult;

    //interpolate
    theta *= t;
    pitch *= t;

    //back to quaternion
    return {
        l * sin(theta/2) + DualQuat(0,0,0,cos(theta/2)),
        m * sin(theta/2) + pitch / 2 * cos(theta/2) * l + DualQuat(0,0,0,-pitch/2*sin(theta/2))
    };
}
