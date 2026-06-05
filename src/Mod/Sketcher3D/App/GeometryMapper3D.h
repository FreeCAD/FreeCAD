// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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


#pragma once

#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

#include "GeoEnum3D.h"


namespace Part
{
class Geometry;
}

namespace Sketcher3D
{

class Constraint3D;
class Solver3D;

/** Translation layer between Sketch3DObject stored Geometry and
 *  Constraint3D lists and the lower level Solver3D handles. A
 *  mapper instance is built per recompute.
 */
class Sketcher3DExport GeometryMapper3D
{
public:
    GeometryMapper3D();
    ~GeometryMapper3D();

    /// Set the sketch up with geometry and constraints.
    void setUpSketch(
        const std::vector<Part::Geometry*>& geoList,
        const std::vector<Constraint3D>& constraints,
        Solver3D& solver
    );

    bool hasMalformedConstraints() const
    {
        return !MalformedConstraints.empty();
    }

    const std::vector<int>& getMalformedConstraints() const
    {
        return MalformedConstraints;
    }

    /// Update the owned geometry copies from the applied solver solution.
    void updateGeometry(const Solver3D& solver);

    /// Return clones of the current internal geometry, preserving GeoId slots.
    std::vector<Part::Geometry*> extractGeometry() const;

private:
    enum GeoType
    {
        None = 0,
        Point,
        Line
    };

    struct GeoDef
    {
        std::unique_ptr<Part::Geometry> geo;
        GeoType type = None;    ///< Type of the geometry
        int index = -1;         ///< Index in the corresponding storage vector (Lines, Arcs, ...)
        int startPointId = -1;  ///< Index in Points of the start point of this geometry
        int midPointId = -1;    ///< Index in Points of the mid point of this geometry
        int endPointId = -1;    ///< Index in Points of the end point of this geometry
    };

    void clear(Solver3D& solver);
    void addGeometry(const std::vector<Part::Geometry*>& geoList, Solver3D& solver);
    int addConstraints(const std::vector<Constraint3D>& constraints, Solver3D& solver);

    /// Return tagId when the constraint was mapped, or -1 when malformed.
    int addConstraint(const Constraint3D& constraint, int tagId, Solver3D& solver);

    /// Return the solver point index for a geometry.
    int getPointId(const GeoElementId3D& ref) const;

    /// Return the solver line index for a geometry.
    int getLineId(const GeoElementId3D& ref) const;

    std::vector<GeoDef> Geoms;
    int rootPointId {-1};
    std::vector<int> MalformedConstraints;
};

}  // namespace Sketcher3D
