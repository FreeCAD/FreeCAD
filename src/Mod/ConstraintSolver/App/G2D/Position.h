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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_POSITION_H
#define FREECAD_CONSTRAINTSOLVER_G2D_POSITION_H

#include "Vector.h"

namespace FCS{ namespace G2D {

/**
 * @brief The Position class is similar to Vector, with the difference being how it is treated by transforms.
 *
 * Positions are transformed by both translations and rotations. Vectors are only transformed by rotations.
 */

class /*FCSExport*/ Position
{
public: //data
    Base::DualNumber x;
    Base::DualNumber y;

public: //methods
    Position() = default;
    Position(Base::DualNumber x, Base::DualNumber y) : x(x), y(y) {}
    Position(const ValueSet& vals, ParameterRef x, ParameterRef y);
    explicit Position(Vector vec){x = vec.x; y = vec.y;}

    Position operator-() const {return Position(-x, -y);}
    Base::DualNumber operator[](int index) const;
    operator Vector() const{return Vector(x,y);}

    PyObject* getPyObject() const;
};

inline Vector operator-(Position a, Position b){
    return Vector(a.x - b.x, a.y - b.y);
}
inline Position operator-(Position a, Vector b){
    return Position(a.x - b.x, a.y - b.y);
}
inline Position operator+(Position a, Position b){
    return Position(a.x + b.x, a.y + b.y);
}
inline Position operator+(Position a, Vector b){
    return Position(a.x + b.x, a.y + b.y);
}
inline Position operator*(Position a, Base::DualNumber b){
    return Position(a.x * b, a.y * b);
}
inline Position operator*(Base::DualNumber a, Position b){
    return b * a;
}


}} //namespace

#endif
