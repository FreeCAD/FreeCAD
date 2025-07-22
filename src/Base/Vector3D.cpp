/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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

#ifndef _PreComp_
#include <limits>
#endif

#include <cmath>
#include <boost/math/special_functions/fpclassify.hpp>

#include "Vector3D.h"
#include "Tools.h"


using namespace Base;

template<class float_type>
Vector3<float_type>::Vector3(float_type fx, float_type fy, float_type fz)
    : x(fx)
    , y(fy)
    , z(fz)
{}

template<class float_type>
float_type& Vector3<float_type>::operator[](unsigned short usIndex)
{
    switch (usIndex) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
    }
    return x;
}

template<class float_type>
const float_type& Vector3<float_type>::operator[](unsigned short usIndex) const
{
    switch (usIndex) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
    }
    return x;
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::operator+(const Vector3<float_type>& rcVct) const
{
    Vector3<float_type> cVctRes;
    cVctRes.x = x + rcVct.x;
    cVctRes.y = y + rcVct.y;
    cVctRes.z = z + rcVct.z;
    return cVctRes;
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::operator&(const Vector3<float_type>& rcVct) const
{
    Vector3<float_type> cVctRes;
    cVctRes.x = x * (float_type)fabs(rcVct.x);
    cVctRes.y = y * (float_type)fabs(rcVct.y);
    cVctRes.z = z * (float_type)fabs(rcVct.z);
    return cVctRes;
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::operator-(const Vector3<float_type>& rcVct) const
{
    Vector3<float_type> cVctRes;
    cVctRes.x = x - rcVct.x;
    cVctRes.y = y - rcVct.y;
    cVctRes.z = z - rcVct.z;
    return cVctRes;
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::operator-() const
{
    return Vector3(-x, -y, -z);
}

template<class float_type>
Vector3<float_type>& Vector3<float_type>::operator+=(const Vector3<float_type>& rcVct)
{
    x += rcVct.x;
    y += rcVct.y;
    z += rcVct.z;
    return *this;
}

template<class float_type>
Vector3<float_type>& Vector3<float_type>::operator-=(const Vector3<float_type>& rcVct)
{
    x -= rcVct.x;
    y -= rcVct.y;
    z -= rcVct.z;
    return *this;
}

template<class float_type>
Vector3<float_type>& Vector3<float_type>::operator*=(float_type fScale)
{
    x *= fScale;
    y *= fScale;
    z *= fScale;
    return *this;
}

template<class float_type>
Vector3<float_type>& Vector3<float_type>::operator/=(float_type fDiv)
{
    x /= fDiv;
    y /= fDiv;
    z /= fDiv;
    return *this;
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::operator*(float_type fScale) const
{
    return Vector3<float_type>(this->x * fScale, this->y * fScale, this->z * fScale);
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::operator/(float_type fDiv) const
{
    return Vector3<float_type>(this->x / fDiv, this->y / fDiv, this->z / fDiv);
}

template<class float_type>
float_type Vector3<float_type>::operator*(const Vector3<float_type>& rcVct) const
{
    return (x * rcVct.x) + (y * rcVct.y) + (z * rcVct.z);
}

template<class float_type>
float_type Vector3<float_type>::Dot(const Vector3<float_type>& rcVct) const
{
    return (x * rcVct.x) + (y * rcVct.y) + (z * rcVct.z);
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::operator%(const Vector3<float_type>& rcVct) const
{
    Vector3<float_type> cVctRes;
    cVctRes.x = (y * rcVct.z) - (z * rcVct.y);
    cVctRes.y = (z * rcVct.x) - (x * rcVct.z);
    cVctRes.z = (x * rcVct.y) - (y * rcVct.x);
    return cVctRes;
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::Cross(const Vector3<float_type>& rcVct) const
{
    Vector3<float_type> cVctRes;
    cVctRes.x = (y * rcVct.z) - (z * rcVct.y);
    cVctRes.y = (z * rcVct.x) - (x * rcVct.z);
    cVctRes.z = (x * rcVct.y) - (y * rcVct.x);
    return cVctRes;
}

template<class float_type>
bool Vector3<float_type>::IsOnLineSegment(const Vector3<float_type>& startVct,
                                          const Vector3<float_type>& endVct) const
{
    Vector3<float_type> vectorAB = endVct - startVct;
    Vector3<float_type> vectorAC = *this - startVct;
    Vector3<float_type> crossproduct = vectorAB.Cross(vectorAC);
    float_type dotproduct = vectorAB.Dot(vectorAC);

    if (crossproduct.Length() > traits_type::epsilon()) {
        return false;
    }

    if (dotproduct < 0) {
        return false;
    }

    if (dotproduct > vectorAB.Sqr()) {
        return false;
    }

    return true;
}

template<class float_type>
bool Vector3<float_type>::operator!=(const Vector3<float_type>& rcVct) const
{
    return !((*this) == rcVct);
}

template<class float_type>
bool Vector3<float_type>::operator==(const Vector3<float_type>& rcVct) const
{
    return (std::fabs(x - rcVct.x) <= traits_type::epsilon())
        && (std::fabs(y - rcVct.y) <= traits_type::epsilon())
        && (std::fabs(z - rcVct.z) <= traits_type::epsilon());
}

template<class float_type>
bool Vector3<float_type>::IsEqual(const Vector3<float_type>& rclPnt, float_type tol) const
{
    return Distance(*this, rclPnt) <= tol;
}

template<class float_type>
bool Vector3<float_type>::IsParallel(const Vector3<float_type>& rclDir, float_type tol) const
{
    float_type angle = GetAngle(rclDir);
    if (boost::math::isnan(angle)) {
        return false;
    }

    return angle <= tol || traits_type::pi() - angle <= tol;
}

template<class float_type>
bool Vector3<float_type>::IsNormal(const Vector3<float_type>& rclDir, float_type tol) const
{
    float_type angle = GetAngle(rclDir);
    if (boost::math::isnan(angle)) {
        return false;
    }

    float_type diff = std::abs(traits_type::pi() / 2.0 - angle);  // NOLINT
    return diff <= tol;
}

template<class float_type>
Vector3<float_type>& Vector3<float_type>::ProjectToPlane(const Vector3<float_type>& rclBase,
                                                         const Vector3<float_type>& rclNorm)
{
    Vector3<float_type> clTemp(rclNorm);
    *this = *this - (clTemp *= ((*this - rclBase) * clTemp) / clTemp.Sqr());
    return *this;
}

template<class float_type>
void Vector3<float_type>::ProjectToPlane(const Vector3& rclBase,
                                         const Vector3& rclNorm,
                                         Vector3& rclProj) const
{
    Vector3<float_type> clTemp(rclNorm);
    rclProj = *this - (clTemp *= ((*this - rclBase) * clTemp) / clTemp.Sqr());
}

template<class float_type>
float_type Vector3<float_type>::DistanceToPlane(const Vector3<float_type>& rclBase,
                                                const Vector3<float_type>& rclNorm) const
{
    return ((*this - rclBase) * rclNorm) / rclNorm.Length();
}

template<class float_type>
float_type Vector3<float_type>::Length() const
{
    return static_cast<float_type>(std::sqrt((x * x) + (y * y) + (z * z)));
}

template<class float_type>
float_type Vector3<float_type>::DistanceToLine(const Vector3<float_type>& base,
                                               const Vector3<float_type>& dir) const
{
    // clang-format off
    return static_cast<float_type>(std::fabs((dir % Vector3(*this - base)).Length() / dir.Length()));
    // clang-format on
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::DistanceToLineSegment(const Vector3& rclP1,
                                                               const Vector3& rclP2) const
{
    float_type len2 = Base::DistanceP2(rclP1, rclP2);
    if (len2 == 0) {
        return rclP1;
    }

    Vector3<float_type> p2p1 = rclP2 - rclP1;
    Vector3<float_type> pXp1 = *this - rclP1;
    float_type dot = pXp1 * p2p1;
    float_type t = clamp<float_type>(dot / len2, 0, 1);
    Vector3<float_type> dist = t * p2p1 - pXp1;
    return dist;
}

template<class float_type>
Vector3<float_type>& Vector3<float_type>::ProjectToLine(const Vector3<float_type>& rclPoint,
                                                        const Vector3<float_type>& rclLine)
{
    return (*this = ((((rclPoint * rclLine) / rclLine.Sqr()) * rclLine) - rclPoint));
}

template<class float_type>
Vector3<float_type> Vector3<float_type>::Perpendicular(const Vector3<float_type>& rclBase,
                                                       const Vector3<float_type>& rclDir) const
{
    float_type t = ((*this - rclBase) * rclDir) / (rclDir * rclDir);
    return rclBase + t * rclDir;
}

template<class float_type>
float_type Vector3<float_type>::Sqr() const
{
    return (float_type)((x * x) + (y * y) + (z * z));
}

template<class float_type>
void Vector3<float_type>::Set(float_type fX, float_type fY, float_type fZ)
{
    x = fX;
    y = fY;
    z = fZ;
}

template<class float_type>
void Vector3<float_type>::ScaleX(float_type f)
{
    x *= f;
}

template<class float_type>
void Vector3<float_type>::ScaleY(float_type f)
{
    y *= f;
}

template<class float_type>
void Vector3<float_type>::ScaleZ(float_type f)
{
    z *= f;
}

template<class float_type>
void Vector3<float_type>::Scale(float_type fX, float_type fY, float_type fZ)
{
    x *= fX;
    y *= fY;
    z *= fZ;
}

template<class float_type>
void Vector3<float_type>::MoveX(float_type f)
{
    x += f;
}

template<class float_type>
void Vector3<float_type>::MoveY(float_type f)
{
    y += f;
}

template<class float_type>
void Vector3<float_type>::MoveZ(float_type f)
{
    z += f;
}

template<class float_type>
void Vector3<float_type>::Move(float_type fX, float_type fY, float_type fZ)
{
    x += fX;
    y += fY;
    z += fZ;
}

template<class float_type>
void Vector3<float_type>::RotateX(float_type f)
{
    Vector3 cPt(*this);

    float_type fsin = static_cast<float_type>(sin(f));
    float_type fcos = static_cast<float_type>(cos(f));
    y = (cPt.y * fcos) - (cPt.z * fsin);
    z = (cPt.y * fsin) + (cPt.z * fcos);
}

template<class float_type>
void Vector3<float_type>::RotateY(float_type f)
{
    Vector3 cPt(*this);

    float_type fsin = static_cast<float_type>(sin(f));
    float_type fcos = static_cast<float_type>(cos(f));
    x = (cPt.z * fsin) + (cPt.x * fcos);
    z = (cPt.z * fcos) - (cPt.x * fsin);
}

template<class float_type>
void Vector3<float_type>::RotateZ(float_type f)
{
    Vector3 cPt(*this);

    float_type fsin = static_cast<float_type>(sin(f));
    float_type fcos = static_cast<float_type>(cos(f));
    x = (cPt.x * fcos) - (cPt.y * fsin);
    y = (cPt.x * fsin) + (cPt.y * fcos);
}

template<class float_type>
Vector3<float_type>& Vector3<float_type>::Normalize()
{
    float_type fLen = Length();
    if (fLen != static_cast<float_type>(0.0) && fLen != static_cast<float_type>(1.0)) {
        x /= fLen;
        y /= fLen;
        z /= fLen;
    }
    return *this;
}

template<class float_type>
bool Vector3<float_type>::IsNull() const
{
    float_type n {0.0};
    return (x == n) && (y == n) && (z == n);
}

template<class float_type>
float_type Vector3<float_type>::GetAngle(const Vector3& rcVect) const
{
    float_type len1 = Length();
    float_type len2 = rcVect.Length();
    if (len1 <= traits_type::epsilon() || len2 <= traits_type::epsilon()) {
        return std::numeric_limits<float_type>::quiet_NaN();  // division by zero
    }

    float_type dot = Dot(rcVect);
    dot /= len1;
    dot /= len2;

    if (dot <= -1.0) {
        return traits_type::pi();
    }
    if (dot >= 1.0) {
        return 0.0;
    }

    return float_type(acos(dot));
}

template<class float_type>
float_type Vector3<float_type>::GetAngleOriented(const Vector3& rcVect, const Vector3& norm) const
{
    float_type angle = GetAngle(rcVect);

    Vector3<float_type> crossProduct = Cross(rcVect);

    // Use dot product to determine the sign
    float_type dot = crossProduct.Dot(norm);
    if (dot < 0) {
        angle = 2 * traits_type::pi() - angle;
    }

    return angle;
}

template<class float_type>
void Vector3<float_type>::TransformToCoordinateSystem(const Vector3& rclBase,
                                                      const Vector3& rclDirX,
                                                      const Vector3& rclDirY)
{
    Vector3 clVectX;
    Vector3 clVectY;
    Vector3 clVectZ;
    Vector3 clVectOld;

    clVectX = rclDirX;
    clVectY = rclDirY;
    clVectZ = rclDirX % rclDirY;
    clVectX.Normalize();
    clVectY.Normalize();
    clVectZ.Normalize();

    clVectOld = *this - rclBase;

    x = clVectX * clVectOld;
    y = clVectY * clVectOld;
    z = clVectZ * clVectOld;
}

// explicit template instantiation
namespace Base
{
template class BaseExport Vector3<float>;
template class BaseExport Vector3<double>;
}  // namespace Base
