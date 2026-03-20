// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include "Axis.h"

namespace Base
{
class Rotation;

/**
 * Describes a right-handed coordinate system in 3D space.
 \author Werner Mayer
 */
class BaseExport CoordinateSystem
{
public:
    /** Construct a default coordinate system with position in (0,0,0),
     * with X axis (1,0,0), with Y axis (0,1,0) and Z axis (0,0,1)
     */
    CoordinateSystem();
    CoordinateSystem(const CoordinateSystem&) = default;
    CoordinateSystem(CoordinateSystem&&) = default;
    ~CoordinateSystem() = default;

    CoordinateSystem& operator=(const CoordinateSystem&) = default;
    CoordinateSystem& operator=(CoordinateSystem&&) = default;

    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis \a v must not be parallel to the X axis
     */
    void setAxis(const Axis& axis);
    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis must not be parallel to \a xd
     */
    void setAxes(const Axis&, const Vector3d& xd);
    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis \a n must not be parallel to \a xd
     */
    void setAxes(const Vector3d& n, const Vector3d& xd);
    inline const Axis& getAxis() const
    {
        return axis;
    }

    /** The passed vector must not be parallel to the main axis */
    void setXDirection(const Vector3d&);
    inline const Vector3d& getXDirection() const
    {
        return xdir;
    }

    /** The passed vector must not be parallel to the main axis */
    void setYDirection(const Vector3d&);
    inline const Vector3d& getYDirection() const
    {
        return ydir;
    }

    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis must not be parallel to the X axis
     */
    void setZDirection(const Vector3d&);
    inline const Vector3d& getZDirection() const
    {
        return axis.getDirection();
    }
    inline void setPosition(const Vector3d& pos)
    {
        axis.setBase(pos);
    }
    inline const Vector3d& getPosition() const
    {
        return axis.getBase();
    }

    /** This computes the displacement from this coordinate system to the
     * given coordinate system \a cs
     */
    Placement displacement(const CoordinateSystem& cs) const;

    /** Transform the point \a pnt to be in this coordinate system */
    void transformTo(Vector3d& pnt);

    /** Apply the placement \a plm to the coordinate system. */
    void transform(const Placement& plm);

    /** Apply the rotation \a rot to the coordinate system. */
    void transform(const Rotation& rot);

    /** Set the placement \a plm to the coordinate system. */
    void setPlacement(const Placement& plm);

private:
    Axis axis;
    Vector3d xdir;
    Vector3d ydir;
};

}  // namespace Base
