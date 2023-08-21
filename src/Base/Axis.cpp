/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "Axis.h"

using namespace Base;

Axis::Axis(const Vector3d& Orig, const Vector3d& Dir)
  : _base{Orig}
  , _dir{Dir}
{
}

void Axis::reverse()
{
    this->_dir = -this->_dir;
}

Axis Axis::reversed() const
{
    Axis a(*this);
    a.reverse();
    return a;
}

void Axis::move(const Vector3d& MovVec)
{
    _base += MovVec;
}

bool Axis::operator ==(const Axis& that) const
{
    return (this->_base == that._base) && (this->_dir == that._dir);
}

bool Axis::operator !=(const Axis& that) const
{
    return !(*this == that);
}

Axis& Axis::operator *=(const Placement &p)
{
    p.multVec(this->_base, this->_base);
    p.getRotation().multVec(this->_dir, this->_dir);
    return *this;
}

Axis Axis::operator *(const Placement &p) const
{
    Axis a(*this);
    a *= p;
    return a;
}
