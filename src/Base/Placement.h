/***************************************************************************
 *   Copyright (c) 2006 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef BASE_PLACEMENT_H
#define BASE_PLACEMENT_H

#include "Rotation.h"
#include "Vector3D.h"


namespace Base {

class DualQuat;
class Matrix4D;

/**
 * The Placement class.
 */
class BaseExport Placement
{
public:
    /// default constructor
    Placement();
    Placement(const Placement&) = default;
    Placement(Placement&&) = default;
    Placement(const Base::Matrix4D& matrix);
    Placement(const Vector3d& Pos, const Rotation &Rot);
    Placement(const Vector3d& Pos, const Rotation &Rot, const Vector3d& Cnt);

    /** specialty constructors */
    //@{
    static Placement fromDualQuaternion(DualQuat qq);
    //@}

    /// Destruction
    ~Placement () = default;

    Matrix4D toMatrix() const;
    void fromMatrix(const Matrix4D& m);
    DualQuat toDualQuaternion() const;
    const Vector3d& getPosition() const {return _pos;}
    const Rotation& getRotation() const {return _rot;}
    void setPosition(const Vector3d& Pos){_pos=Pos;}
    void setRotation(const Rotation& Rot) {_rot = Rot;}

    bool isIdentity() const;
    bool isIdentity(double tol) const;
    void invert();
    Placement inverse() const;
    void move(const Vector3d& MovVec);

    bool isSame(const Placement&) const;
    bool isSame(const Placement&, double tol) const;

    /** Operators. */
    //@{
    Placement & operator*=(const Placement & p);
    Placement operator *(const Placement & p) const;
    bool operator == (const Placement&) const;
    bool operator != (const Placement&) const;
    Placement& operator = (const Placement&) = default;
    Placement& operator = (Placement&&) = default;
    Placement pow(double t, bool shorten = true) const;

    Placement& multRight(const Base::Placement& p);
    Placement& multLeft(const Base::Placement& p);

    void multVec(const Vector3d & src, Vector3d & dst) const;
    void multVec(const Vector3f & src, Vector3f & dst) const;
    //@}

    static Placement slerp(const Placement & p0, const Placement & p1, double t);
    static Placement sclerp(const Placement & p0, const Placement & p1, double t, bool shorten = true);

private:
    Vector3<double> _pos;
    Base::Rotation  _rot;
};

} // namespace Base


#endif // BASE_PLACEMENT_H


