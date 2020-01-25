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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_POINT_H
#define FREECAD_CONSTRAINTSOLVER_G2D_POINT_H

#include "Vector.h"

namespace FCS{ namespace G2D {

/**
 * @brief The Point class is similar to Vector, with the difference being how it is treated by transforms.
 *
 * Points are transformed by both translations and rotations. Vectors are only transformed by rotations.
 */

class /*FCSExport*/ Point
{
public: //data
    Base::DualNumber x;
    Base::DualNumber y;

public: //methods
    Point() = default;
    Point(Base::DualNumber x, Base::DualNumber y) : x(x), y(y) {}
    Point(const ValueSet& vals, ParameterRef x, ParameterRef y);
    explicit Point(Vector vec){x = vec.x; y = vec.y;}

    Point operator-() const {return Point(-x, -y);}
    Base::DualNumber operator[](int index) const;
    operator Vector() const{return Vector(x,y);}

    PyObject* getPyObject() const;
};

inline Vector operator-(Point a, Point b){
    return Vector(a.x - b.x, a.y - b.y);
}
inline Point operator-(Point a, Vector b){
    return Point(a.x - b.x, a.y - b.y);
}
inline Point operator+(Point a, Point b){
    return Point(a.x + b.x, a.y + b.y);
}
inline Point operator+(Point a, Vector b){
    return Point(a.x + b.x, a.y + b.y);
}
inline Point operator*(Point a, Base::DualNumber b){
    return Point(a.x * b, a.y * b);
}
inline Point operator*(Base::DualNumber a, Point b){
    return b * a;
}


}} //namespace

#endif
