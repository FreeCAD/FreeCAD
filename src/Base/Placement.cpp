/***************************************************************************
 *   Copyright (c) 2006 Juergen Riegel                                     *
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
#endif


#include "Placement.h"
#include "Rotation.h"

using namespace Base;

Placement::Placement()
{

}

Placement::Placement(const Base::Matrix4D& matrix)
{
    fromMatrix(matrix);
}

Placement::Placement(const Placement& that)
{
    this->_pos = that._pos;
    this->_rot = that._rot;
}

Placement::Placement(const Vector3d& Pos, const Rotation &Rot)
{
    this->_pos = Pos;
    this->_rot = Rot;
}

Placement::Placement(const Vector3d& Pos, const Rotation &Rot, const Vector3d& Cnt)
{
    Vector3d RotC = Cnt;
    Rot.multVec(RotC, RotC);
    this->_pos = Pos + Cnt - RotC;
    this->_rot = Rot;
}

Base::Matrix4D Placement::toMatrix(void) const
{
    Base::Matrix4D matrix;
    _rot.getValue(matrix);
    matrix[0][3] = this->_pos.x;
    matrix[1][3] = this->_pos.y;
    matrix[2][3] = this->_pos.z;
    return matrix;
}

void Placement::fromMatrix(const Base::Matrix4D& matrix)
{
    _rot.setValue(matrix);
    this->_pos.x = matrix[0][3];
    this->_pos.y = matrix[1][3];
    this->_pos.z = matrix[2][3];
}

void Placement::invert()
{
    this->_rot = this->_rot.inverse();
    this->_rot.multVec(this->_pos, this->_pos);
    this->_pos = -this->_pos;
}

Placement Placement::inverse() const
{
    Placement p(*this);
    p.invert();
    return p;
}

void Placement::move(const Vector3d& MovVec)
{
    _pos += MovVec;
}

bool Placement::operator == (const Placement& that) const
{
    return (this->_pos == that._pos) && (this->_rot == that._rot);
}

bool Placement::operator != (const Placement& that) const
{
    return !(*this == that);
}

Placement & Placement::operator*=(const Placement & p)
{
    Base::Vector3d tmp(p._pos);
    this->_rot.multVec(tmp, tmp);
    this->_pos += tmp;
    this->_rot *= p._rot;
    return *this;
}

Placement Placement::operator*(const Placement & p) const
{
    Placement plm(*this);
    plm *= p;
    return plm;
}

Placement& Placement::operator = (const Placement& New)
{
    this->_pos = New._pos;
    this->_rot = New._rot;
    return *this;
}

void Placement::multVec(const Vector3d & src, Vector3d & dst) const
{
    this->_rot.multVec(src, dst);
    dst += this->_pos;
}

Placement Placement::slerp(const Placement & p0, const Placement & p1, double t)
{
    Rotation rot = Rotation::slerp(p0.getRotation(), p1.getRotation(), t);
    Vector3d pos = p0.getPosition() * (1.0-t) + p1.getPosition() * t;
    return Placement(pos, rot);
}
