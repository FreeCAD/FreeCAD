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


#ifndef SKETCHER3D_GEOMETRYMAPPER3D_H
#define SKETCHER3D_GEOMETRYMAPPER3D_H

#include <map>
#include <vector>

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
 *  mapper instance is built per recompute, populated via push(), then
 *  discarded after writeBack().
 */
class Sketcher3DExport GeometryMapper3D
{
public:
    GeometryMapper3D() = default;

    /// Push geometry and constraints into the solver.
    /// Constraints referring to null geometry are silently
    /// dropped.
    void push(
        const std::vector<Part::Geometry*>& geoList,
        const std::vector<Constraint3D>& constraints,
        Solver3D& solver
    );

    /// After solver has run and applied, copy solved positions back into
    /// geoList.
    void writeBack(const Solver3D& solver, std::vector<Part::Geometry*>& geoList) const;

private:
    struct GeoMapping
    {
        int pointHandle {-1};  // GeomPoint
        int startHandle {-1};  // GeomLineSegment start
        int endHandle {-1};    // GeomLineSegment end
        int lineHandle {-1};   // GeomLineSegment line
    };

    /// Resolve a constraint to a solver point
    int resolvePointHandle(const GeoElementId3D& ref) const;

    /// Resolve a constraint to a solver line handle.
    int resolveLineHandle(const GeoElementId3D& ref) const;

    std::map<int, GeoMapping> perGeo;
};

}  // namespace Sketcher3D

#endif  // SKETCHER3D_GEOMETRYMAPPER3D_H
