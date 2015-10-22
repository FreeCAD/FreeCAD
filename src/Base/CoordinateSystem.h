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


#ifndef BASE_COORDINATESYSTEM_H
#define BASE_COORDINATESYSTEM_H

#include "Axis.h"

namespace Base {
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
    ~CoordinateSystem();

    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis \a v must not be parallel to the X axis
     */
    void setAxis(const Axis& v);
    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis must not be parallel to \a xd
     */
    void setAxes(const Axis&, const Vector3d& xd);
    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis \a n must not be parallel to \a xd
     */
    void setAxes(const Vector3d& n, const Vector3d& xd);
    inline const Axis& getAxis() const
    { return axis; }

    /** The passed vector must not be parallel to the main axis */
    void setXDirection(const Vector3d&);
    inline const Vector3d& getXDirection() const
    { return xdir; }

    /** The passed vector must not be parallel to the main axis */
    void setYDirection(const Vector3d&);
    inline const Vector3d& getYDirection() const
    { return ydir; }

    /** Sets the main axis. X and Y dir are adjusted accordingly.
     * The main axis must not be parallel to the X axis
     */
    void setZDirection(const Vector3d&);
    inline const Vector3d& getZDirection() const
    { return axis.getDirection(); }
    inline void setPosition(const Vector3d& p)
    { axis.setBase(p); }
    inline const Vector3d& getPosition() const
    { return axis.getBase(); }

    /** This computes the displacement from this coordinate system to the
     * given coordinate system \a cs
     */
    Placement displacement(const CoordinateSystem& cs) const;

    /** Transform the point \a p to be in this coordinate system */
    void transformTo(Vector3d& p);

    /** Apply the placement \a p to the coordinate system. */
    void transform(const Placement& p);

    /** Apply the rotation \a r to the coordinate system. */
    void transform(const Rotation& r);

    /** Set the placement \a p to the coordinate system. */
    void setPlacement(const Placement& p);

private:
    Axis axis;
    Vector3d xdir;
    Vector3d ydir;
};

}

#endif // BASE_COORDINATESYSTEM_H
