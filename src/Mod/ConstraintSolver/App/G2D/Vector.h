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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_VECTOR_H
#define FREECAD_CONSTRAINTSOLVER_G2D_VECTOR_H

#include "../Utils.h" //temporary
#include <Base/DualNumber.h>

namespace FCS {
    class ParameterRef;
    class ValueSet;
}

namespace FCS{ namespace G2D {

class /*FCSExport*/ Vector
{
public: //data
    Base::DualNumber x;
    Base::DualNumber y;

public: //methods
    Vector() = default;
    Vector(Base::DualNumber x, Base::DualNumber y) : x(x), y(y) {}
    Vector(const ValueSet& vals, ParameterRef x, ParameterRef y);

    Base::DualNumber sqlength() const {return x*x + y*y;}
    Base::DualNumber length() const;
    Vector normalized() const;
    Vector rotate90ccw() const {return Vector(-y, x);}
    Vector rotate90cw() const {return Vector(y, -x);}
    static Base::DualNumber dot(Vector a, Vector b) {return a.x * b.x + a.y * b.y;}
    static Base::DualNumber cross(Vector a, Vector b) {return dot(a, b.rotate90cw());}
    Vector operator-() const {return Vector(-x, -y);}
    Base::DualNumber operator[](int index) const;

    PyObject* getPyObject() const;
    std::string repr() const;
};

inline Vector operator-(Vector a, Vector b){
    return Vector(a.x - b.x, a.y - b.y);
}
inline Vector operator+(Vector a, Vector b){
    return Vector(a.x + b.x, a.y + b.y);
}
inline Vector operator*(Vector a, Base::DualNumber b){
    return Vector(a.x * b, a.y * b);
}
inline Vector operator*(Base::DualNumber a, Vector b){
    return b*a;
}
inline Vector operator/(Vector a, Base::DualNumber b){
    return Vector(a.x / b, a.y / b);
}

}} //namespace

#endif
