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


#ifndef BASE_TOOLS3D_H
#define BASE_TOOLS3D_H


#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <list>
#include <vector>

#include <Base/BoundBox.h>
#include <Base/Placement.h>
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif

namespace Base {

class Vector2d;
class BoundBox2d;
class Line2d;
class Polygon2d;

/** Line3 ********************************************/

/**
 * 3D line class.
 */
template <class float_type>
class Line3
{
public:
    Vector3<float_type> p1, p2;

    Line3() = default;
    ~Line3() = default;
    Line3(const Line3& line);
    Line3(Line3&& line);
    Line3(const Vector3<float_type>& p1, const Vector3<float_type>& p2);

    // operators
    Line3& operator= (const Line3& line);
    Line3& operator= (Line3&& line);
    bool operator== (const Line3& line) const;

    // methods
    float_type Length () const;
    float_type SqrLength () const;
    BoundBox3<float_type> CalcBoundBox() const;
    Vector3<float_type> GetBase() const;
    Vector3<float_type> GetDirection() const;
    void Transform(const Base::Matrix4D&);
    void Transform(const Base::Placement&);
    void Transform(const Base::Rotation&);
    Line3 Transformed(const Base::Matrix4D&) const;
    Line3 Transformed(const Base::Placement&) const;
    Line3 Transformed(const Base::Rotation&) const;

    /*!
     * \brief Contains
     * Checks if the point \a p is part of the line segment.
     * \param p
     * \return
     */
    bool Contains(const Vector3<float_type>& p) const;
    /*!
     * \brief Contains
     * Checks if the distance of point \a p to the line segment is
     * less than \a eps
     * \param p
     * \param eps
     * \return
     */
    bool Contains(const Vector3<float_type>& p, float_type eps) const;
    Vector3<float_type> FromPos (float_type distance) const;
};

/** Polygon3 ********************************************/

/**
 * 3D polygon class.
 */
template <class float_type>
class Polygon3
{
public:
    Polygon3() = default;
    ~Polygon3() = default;
    Polygon3(const Polygon3<float_type>& poly) = default;

    Polygon3& operator = (const Polygon3<float_type>& poly) = default;

    size_t GetSize() const;
    void Add (const Vector3<float_type>& p);
    const Vector3<float_type>& operator[] (size_t pos) const;
    const Vector3<float_type>& At (size_t pos) const;
    Vector3<float_type>& operator[] (size_t pos);
    Vector3<float_type>& At (size_t pos);
    bool Remove(size_t pos);
    void Clear();

    // misc
    BoundBox3<float_type> CalcBoundBox() const;
    void Transform(const Base::Matrix4D&);
    void Transform(const Base::Placement&);
    void Transform(const Base::Rotation&);
    Polygon3 Transformed(const Base::Matrix4D&) const;
    Polygon3 Transformed(const Base::Placement&) const;
    Polygon3 Transformed(const Base::Rotation&) const;

private:
    std::vector<Vector3<float_type>> points;
};

using Line3f = Line3<float>;
using Line3d = Line3<double>;

using Polygon3f = Polygon3<float>;
using Polygon3d = Polygon3<double>;

} // namespace Base

#endif // BASE_TOOLS3D_H
