/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <cstdlib>
# include <set>
#endif

#include "Tools3D.h"
#include "Vector3D.h"

using namespace Base;


template <typename float_type>
Line3<float_type>::Line3(const Vector3<float_type>& p1, const Vector3<float_type>& p2)
  : p1(p1)
  , p2(p2)
{
}

template <typename float_type>
bool Line3<float_type>::operator== (const Line3<float_type>& line) const
{
    return (p1 == line.p1 && p2 == line.p2);
}

template <typename float_type>
float_type Line3<float_type>::Length() const
{
    return Base::Distance(p1, p2);
}

template <typename float_type>
float_type Line3<float_type>::SqrLength() const
{
    return Base::DistanceP2(p1, p2);
}

template <typename float_type>
BoundBox3<float_type> Line3<float_type>::CalcBoundBox() const
{
    BoundBox3<float_type> box;
    box.Add(p1);
    box.Add(p2);
    return box;
}

template <typename float_type>
Vector3<float_type> Line3<float_type>::GetBase() const
{
    return p1;
}

template <typename float_type>
Vector3<float_type> Line3<float_type>::GetDirection() const
{
    return p2 - p1;
}

template <typename float_type>
void Line3<float_type>::Transform(const Base::Matrix4D& mat)
{
    mat.multVec(p1, p1);
    mat.multVec(p2, p2);
}

template <typename float_type>
void Line3<float_type>::Transform(const Base::Placement& plm)
{
    plm.multVec(p1, p1);
    plm.multVec(p2, p2);
}

template <typename float_type>
void Line3<float_type>::Transform(const Base::Rotation& rot)
{
    rot.multVec(p1, p1);
    rot.multVec(p2, p2);
}

template <typename float_type>
Line3<float_type> Line3<float_type>::Transformed(const Base::Matrix4D& mat) const
{
    Line3<float_type> line(*this);
    line.Transform(mat);
    return line;
}

template <typename float_type>
Line3<float_type> Line3<float_type>::Transformed(const Base::Placement& plm) const
{
    Line3<float_type> line(*this);
    line.Transform(plm);
    return line;
}

template <typename float_type>
Line3<float_type> Line3<float_type>::Transformed(const Base::Rotation& rot) const
{
    Line3<float_type> line(*this);
    line.Transform(rot);
    return line;
}

template <typename float_type>
bool Line3<float_type>::Contains(const Vector3<float_type>& pt) const
{
    return Contains(pt, Vector3<float_type>::epsilon());
}

template <typename float_type>
bool Line3<float_type>::Contains(const Vector3<float_type>& pt, float_type eps) const
{
    Vector3<float_type> v1 = p1 - pt;
    Vector3<float_type> v2 = p2 - pt;
    Vector3<float_type> v3 = p2 - p1;
    float_type dot = v1.Dot(v2);
    if (dot > eps)
        return false;
    return v3.Cross(v1).Length() < eps;
}

template <typename float_type>
Vector3<float_type> Line3<float_type>::FromPos(float_type distance) const
{
    Vector3<float_type> dir(p2 - p1);
    dir.Normalize();
    return p1 + dir * distance;
}

// ----------------------------------------------------------------------------

template <typename float_type>
size_t Polygon3<float_type>::GetSize() const
{
    return points.size();
}

template <typename float_type>
void Polygon3<float_type>::Add (const Vector3<float_type>& p)
{
    points.push_back(p);
}

template <typename float_type>
const Vector3<float_type>& Polygon3<float_type>::operator[] (size_t pos) const
{
    return points[pos];
}

template <typename float_type>
const Vector3<float_type>& Polygon3<float_type>::At (size_t pos) const
{
    return points.at(pos);
}

template <typename float_type>
Vector3<float_type>& Polygon3<float_type>::operator[] (size_t pos)
{
    return points[pos];
}

template <typename float_type>
Vector3<float_type>& Polygon3<float_type>::At (size_t pos)
{
    return points.at(pos);
}

template <typename float_type>
bool Polygon3<float_type>::Remove(size_t pos)
{
    if (pos < points.size()) {
        auto it = points.begin();
        std::advance(it, pos);
        points.erase(it);
        return true;
    }

    return false;
}

template <typename float_type>
void Polygon3<float_type>::Clear()
{
    points.clear();
}

template <typename float_type>
BoundBox3<float_type> Polygon3<float_type>::CalcBoundBox() const
{
    BoundBox3<float_type> box;
    for (const auto& it : points)
        box.Add(it);
    return box;
}

template <typename float_type>
void Polygon3<float_type>::Transform(const Base::Matrix4D& mat)
{
    for (auto& it : points)
        mat.multVec(it, it);
}

template <typename float_type>
void Polygon3<float_type>::Transform(const Base::Placement& plm)
{
    for (auto& it : points)
        plm.multVec(it, it);
}

template <typename float_type>
void Polygon3<float_type>::Transform(const Base::Rotation& rot)
{
    for (auto& it : points)
        rot.multVec(it, it);
}

template <typename float_type>
Polygon3<float_type> Polygon3<float_type>::Transformed(const Base::Matrix4D& mat) const
{
    Polygon3<float_type> poly(*this);
    poly.Transform(mat);
    return poly;
}

template <typename float_type>
Polygon3<float_type> Polygon3<float_type>::Transformed(const Base::Placement& plm) const
{
    Polygon3<float_type> poly(*this);
    poly.Transform(plm);
    return poly;
}

template <typename float_type>
Polygon3<float_type> Polygon3<float_type>::Transformed(const Base::Rotation& rot) const
{
    Polygon3<float_type> poly(*this);
    poly.Transform(rot);
    return poly;
}

// explicit template instantiation
namespace Base {
template class BaseExport Line3<float>;
template class BaseExport Line3<double>;
template class BaseExport Polygon3<float>;
template class BaseExport Polygon3<double>;
}
