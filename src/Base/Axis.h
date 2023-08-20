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


#ifndef BASE_AXIS_H
#define BASE_AXIS_H

#include "Placement.h"
#include "Vector3D.h"

namespace Base {

/**
 * The Axis class.
 */
class BaseExport Axis
{
public:
    /// default constructor
    Axis() = default;
    Axis(const Axis&) = default;
    Axis(Axis&&) = default;
    Axis(const Vector3d& Orig, const Vector3d& Dir);
    /// Destruction
    ~Axis () = default;

    const Vector3d& getBase() const {return _base;}
    const Vector3d& getDirection() const {return _dir;}
    void setBase(const Vector3d& Orig) {_base=Orig;}
    void setDirection(const Vector3d& Dir) {_dir=Dir;}

    void reverse();
    Axis reversed() const;
    void move(const Vector3d& MovVec);

    /** Operators. */
    //@{
    Axis& operator *=(const Placement &p);
    Axis operator *(const Placement &p) const;
    bool operator ==(const Axis&) const;
    bool operator !=(const Axis&) const;
    Axis& operator =(const Axis&) = default;
    Axis& operator =(Axis&&) = default;
    //@}

private:
    Vector3d _base;
    Vector3d _dir;
};

} // namespace Base

#endif // BASE_AXIS_H
