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

#include <limits>

#include "Vector3D.h"
#include "Tools.h"


using namespace Base;

template <class _Precision>
Vector3<_Precision>::Vector3 (_Precision fx, _Precision fy, _Precision fz)
  : x(fx),
    y(fy),
    z(fz)
{
}

template <class _Precision>
_Precision& Vector3<_Precision>::operator [] (unsigned short usIndex)
{
    switch (usIndex)
    {
        case 0: return x;
        case 1: return y;
        case 2: return z;
    }
    return x;
}

template <class _Precision>
const _Precision& Vector3<_Precision>::operator [] (unsigned short usIndex) const
{
    switch (usIndex)
    {
        case 0: return x;
        case 1: return y;
        case 2: return z;
    }
    return x;
}

template <class _Precision>
Vector3 <_Precision>Vector3<_Precision>::operator +  (const Vector3<_Precision>& rcVct) const
{
    Vector3<_Precision> cVctRes;
    cVctRes.x = x + rcVct.x;
    cVctRes.y = y + rcVct.y;
    cVctRes.z = z + rcVct.z;
    return cVctRes;
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::operator &  (const Vector3<_Precision>& rcVct) const
{
    Vector3<_Precision> cVctRes;
    cVctRes.x = x * (_Precision) fabs(rcVct.x);
    cVctRes.y = y * (_Precision) fabs(rcVct.y);
    cVctRes.z = z * (_Precision) fabs(rcVct.z);
    return cVctRes;
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::operator -  (const Vector3<_Precision>& rcVct) const
{
    Vector3<_Precision> cVctRes;
    cVctRes.x = x - rcVct.x;
    cVctRes.y = y - rcVct.y;
    cVctRes.z = z - rcVct.z;
    return cVctRes;
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::operator - () const
{
    return Vector3(-x, -y, -z);
}

template <class _Precision>
Vector3<_Precision>& Vector3<_Precision>::operator += (const Vector3<_Precision>& rcVct)
{
    x += rcVct.x;
    y += rcVct.y;
    z += rcVct.z;
    return *this;
}

template <class _Precision>
Vector3<_Precision>& Vector3<_Precision>::operator -= (const Vector3<_Precision>& rcVct)
{
    x -= rcVct.x;
    y -= rcVct.y;
    z -= rcVct.z;
    return *this;
}

template <class _Precision>
Vector3<_Precision>& Vector3<_Precision>::operator *= (_Precision fScale)
{
    x *= fScale;
    y *= fScale;
    z *= fScale;
    return *this;
}

template <class _Precision>
Vector3<_Precision>& Vector3<_Precision>::operator /= (_Precision fDiv)
{
    x /= fDiv;
    y /= fDiv;
    z /= fDiv;
    return *this;
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::operator * (_Precision fScale) const
{
    return Vector3<_Precision>(this->x*fScale,this->y*fScale,this->z*fScale);
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::operator / (_Precision fDiv) const
{
    return Vector3<_Precision>(this->x/fDiv,this->y/fDiv,this->z/fDiv);
}

template <class _Precision>
_Precision Vector3<_Precision>::operator *  (const Vector3<_Precision>& rcVct) const
{
    return (x * rcVct.x) + (y * rcVct.y) + (z * rcVct.z);
}

template <class _Precision>
_Precision Vector3<_Precision>::Dot (const Vector3<_Precision>& rcVct) const
{
    return (x * rcVct.x) + (y * rcVct.y) + (z * rcVct.z);
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::operator %  (const Vector3<_Precision>& rcVct) const
{
    Vector3<_Precision> cVctRes;
    cVctRes.x = (y * rcVct.z) - (z * rcVct.y);
    cVctRes.y = (z * rcVct.x) - (x * rcVct.z);
    cVctRes.z = (x * rcVct.y) - (y * rcVct.x);
    return cVctRes;
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::Cross(const Vector3<_Precision>& rcVct) const
{
    Vector3<_Precision> cVctRes;
    cVctRes.x = (y * rcVct.z) - (z * rcVct.y);
    cVctRes.y = (z * rcVct.x) - (x * rcVct.z);
    cVctRes.z = (x * rcVct.y) - (y * rcVct.x);
    return cVctRes;
}

template <class _Precision>
bool Vector3<_Precision>::IsOnLineSegment (const Vector3<_Precision>& startVct, const Vector3<_Precision>& endVct) const
{
    Vector3<_Precision> vectorAB = endVct - startVct;
    Vector3<_Precision> vectorAC = *this - startVct;
    Vector3<_Precision> crossproduct = vectorAB.Cross(vectorAC);
    _Precision dotproduct = vectorAB.Dot(vectorAC);

    if (crossproduct.Length() > traits_type::epsilon())
        return false;

    if (dotproduct < 0)
        return false;

    if (dotproduct > vectorAB.Sqr())
        return false;

    return true;
}

template <class _Precision>
bool Vector3<_Precision>::operator != (const Vector3<_Precision>& rcVct) const
{
    return !((*this) == rcVct);
}

template <class _Precision>
bool Vector3<_Precision>::operator == (const Vector3<_Precision>& rcVct) const
{
    return (fabs (x - rcVct.x) <= traits_type::epsilon()) &&
           (fabs (y - rcVct.y) <= traits_type::epsilon()) &&
           (fabs (z - rcVct.z) <= traits_type::epsilon());
}

template <class _Precision>
bool Vector3<_Precision>::IsEqual(const Vector3<_Precision> &rclPnt, _Precision tol) const
{
    return Distance(*this, rclPnt) <= tol;
}

template <class _Precision>
Vector3<_Precision>& Vector3<_Precision>::ProjectToPlane (const Vector3<_Precision> &rclBase,
                                                          const Vector3<_Precision> &rclNorm)
{
    Vector3<_Precision> clTemp(rclNorm);
    *this = *this - (clTemp *= ((*this - rclBase) * clTemp) / clTemp.Sqr());
    return *this;
}

template <class _Precision>
void Vector3<_Precision>::ProjectToPlane (const Vector3 &rclBase,
                                          const Vector3 &rclNorm,
                                          Vector3 &rclProj) const
{
    Vector3<_Precision> clTemp(rclNorm);
    rclProj = *this - (clTemp *= ((*this - rclBase) * clTemp) / clTemp.Sqr());
}

template <class _Precision>
_Precision Vector3<_Precision>::DistanceToPlane (const Vector3<_Precision> &rclBase,
                                                 const Vector3<_Precision> &rclNorm) const
{
    return ((*this - rclBase) * rclNorm) / rclNorm.Length();
}

template <class _Precision>
_Precision Vector3<_Precision>::Length () const
{
    return static_cast<_Precision>(sqrt ((x * x) + (y * y) + (z * z)));
}

template <class _Precision>
_Precision Vector3<_Precision>::DistanceToLine (const Vector3<_Precision> &rclBase,
                                                const Vector3<_Precision> &rclDirect) const
{
    return static_cast<_Precision>(fabs((rclDirect % Vector3(*this - rclBase)).Length() / rclDirect.Length()));
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::DistanceToLineSegment(const Vector3& rclP1,
                                                               const Vector3& rclP2) const
{
    _Precision len2 = Base::DistanceP2(rclP1, rclP2);
    if (len2 == 0)
        return rclP1;

    Vector3<_Precision> p2p1 = rclP2-rclP1;
    Vector3<_Precision> pXp1 = *this-rclP1;
    _Precision dot = pXp1 * p2p1;
    _Precision t = clamp<_Precision>(dot/len2, 0, 1);
    Vector3<_Precision> dist = t * p2p1 - pXp1;
    return dist;
}

template <class _Precision>
Vector3<_Precision>& Vector3<_Precision>::ProjectToLine (const Vector3<_Precision> &rclPoint,
                                                         const Vector3<_Precision> &rclLine)
{
    return (*this = ((((rclPoint * rclLine) / rclLine.Sqr()) * rclLine) - rclPoint));
}

template <class _Precision>
Vector3<_Precision> Vector3<_Precision>::Perpendicular(const Vector3<_Precision> &rclBase,
                                                       const Vector3<_Precision> &rclDir) const
{
    _Precision t = ((*this - rclBase) * rclDir) / (rclDir * rclDir);
    return rclBase + t * rclDir;
}

template <class _Precision>
_Precision Vector3<_Precision>::Sqr () const
{
    return (_Precision) ((x * x) + (y * y) + (z * z));
}

template <class _Precision>
void Vector3<_Precision>::Set (_Precision fX, _Precision fY, _Precision fZ)
{
    x = fX;
    y = fY;
    z = fZ;
}

template <class _Precision>
void Vector3<_Precision>::ScaleX (_Precision f)
{
    x *= f;
}

template <class _Precision>
void Vector3<_Precision>::ScaleY (_Precision f)
{
    y *= f;
}

template <class _Precision>
void Vector3<_Precision>::ScaleZ (_Precision f)
{
    z *= f;
}

template <class _Precision>
void Vector3<_Precision>::Scale (_Precision fX, _Precision fY, _Precision fZ)
{
    x *= fX;
    y *= fY;
    z *= fZ;
}

template <class _Precision>
void Vector3<_Precision>::MoveX (_Precision f)
{
    x += f;
}

template <class _Precision>
void Vector3<_Precision>::MoveY (_Precision f)
{
    y += f;
}

template <class _Precision>
void Vector3<_Precision>::MoveZ (_Precision f)
{
    z += f;
}

template <class _Precision>
void Vector3<_Precision>::Move (_Precision fX, _Precision fY, _Precision fZ)
{
    x += fX;
    y += fY;
    z += fZ;
}

template <class _Precision>
void Vector3<_Precision>::RotateX (_Precision f)
{
    Vector3 cPt (*this);
    _Precision fsin, fcos;

    fsin = static_cast<_Precision>(sin(f));
    fcos = static_cast<_Precision>(cos(f));
    y = (cPt.y * fcos) - (cPt.z * fsin);
    z = (cPt.y * fsin) + (cPt.z * fcos);
}

template <class _Precision>
void Vector3<_Precision>::RotateY (_Precision f)
{
    Vector3 cPt (*this);
    _Precision fsin, fcos;

    fsin = static_cast<_Precision>(sin(f));
    fcos = static_cast<_Precision>(cos(f));
    x = (cPt.z * fsin) + (cPt.x * fcos);
    z = (cPt.z * fcos) - (cPt.x * fsin);
}

template <class _Precision>
void Vector3<_Precision>::RotateZ (_Precision f)
{
    Vector3 cPt (*this);
    _Precision fsin, fcos;

    fsin = static_cast<_Precision>(sin(f));
    fcos = static_cast<_Precision>(cos(f));
    x = (cPt.x * fcos) - (cPt.y * fsin);
    y = (cPt.x * fsin) + (cPt.y * fcos);
}

template <class _Precision>
Vector3<_Precision> & Vector3<_Precision>::Normalize ()
{
    _Precision fLen = Length ();
    if (fLen != static_cast<_Precision>(0.0) && fLen != static_cast<_Precision>(1.0)) {
        x /= fLen;
        y /= fLen;
        z /= fLen;
    }
    return *this;
}

template <class _Precision>
bool Vector3<_Precision>::IsNull() const
{
    _Precision n{0.0};
    return (x == n) && (y == n) && (z == n);
}

template <class _Precision>
_Precision Vector3<_Precision>::GetAngle (const Vector3 &rcVect) const
{
    _Precision len1 = Length();
    _Precision len2 = rcVect.Length();
    if (len1 <= traits_type::epsilon() || len2 <= traits_type::epsilon())
        return std::numeric_limits<_Precision>::quiet_NaN(); // division by zero

    _Precision dot = Dot(rcVect);
    dot /= len1;
    dot /= len2;

    if (dot <= -1.0)
        return traits_type::pi();
    else if (dot >= 1.0)
        return 0.0;

    return _Precision(acos(dot));
}

template <class _Precision>
void Vector3<_Precision>::TransformToCoordinateSystem (const Vector3 &rclBase,
                                                       const Vector3 &rclDirX,
                                                       const Vector3 &rclDirY)
{
    Vector3  clVectX, clVectY, clVectZ, clVectOld;

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
namespace Base {
template class BaseExport Vector3<float>;
template class BaseExport Vector3<double>;
}
