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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PLACEMENT_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PLACEMENT_H

#include "Vector.h"
#include "Point.h"
#include "DualMath.h"

namespace FCS{ namespace G2D {

/**
 * @brief FCS::G2D::Placement class represents a 2d Placement - a combination
 * of translation and rotation, made on dual numbers.
 *
 * Order of operation: placed_point = translated(rotated(local_point))
 *
 * Translation is represented by a Point, - the origin of local coordinate
 * system.
 *
 * Rotation is represenced by a (cos, sin) vector (unit-length). Scaling is not
 * supported, but can potentially be encoded as a length of rotation vector.
 *
 * Placements acts differently on points and on vectors. Vectors are only
 * rotated, while points are translated and rotated.
 */
class /*FCSExport*/ Placement
{
public: //data
    Point translation;
    ///rotation encoded as unit-length vector, similarly to 3d rotation quaternion representation
    Vector rotation = Vector(1.0, 0.0);

public: //methods
    Placement() = default;
    Placement(Point translation, Vector rotation) : translation(translation), rotation(rotation) {}
    Placement(Point translation, DualNumber rotationAngle) : translation(translation), rotation(cos(rotationAngle), sin(rotationAngle)) {}
    Placement(const ValueSet& vals, ParameterRef x, ParameterRef y);

    inline Placement inverse() const;

    ///returns angle in 0..2pi range
    Base::DualNumber unsignedAngle() {return signedAngle() + ((rotation.y < 0.0) ? TURN : 0.0);}
    ///returns angle in -pi..pi range
    Base::DualNumber signedAngle() {return atan2n(rotation.y, rotation.x);}

    Point operator*(Point p) const {return Point(Vector(translation) + *this * Vector(p));}
    Vector operator*(Vector v) const {return Vector(v.x * rotation.x - v.y * rotation.y, v.x * rotation.y + v.y*rotation.x);}
    Placement operator-() const {return inverse();}
};

inline Placement operator*(const Placement& a, const Placement& b){
    return Placement(a * b.translation, a * b.rotation);
}

inline Placement Placement::inverse() const
{
    Placement ret;
    ret.rotation = Vector(rotation.x, -rotation.y);
    ret.translation = ret * -translation;
    return ret;
}

inline void operator*=(Placement& self, const Placement& other)
{
    self = self * other;
}

}} //namespace

#endif
