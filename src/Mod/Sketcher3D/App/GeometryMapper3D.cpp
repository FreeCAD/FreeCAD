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


#include "PreCompiled.h"

#include <Mod/Part/App/Geometry.h>

#include "Constraint3D.h"
#include "GeometryMapper3D.h"
#include "Solver3D.h"


using namespace Sketcher3D;

void GeometryMapper3D::push(
    const std::vector<Part::Geometry*>& geoList,
    const std::vector<Constraint3D>& constraints,
    Solver3D& solver
)
{
    perGeo.clear();

    for (std::size_t i = 0; i < geoList.size(); ++i) {
        const Part::Geometry* g = geoList[i];
        if (!g) {
            continue;
        }
        const int geoId = static_cast<int>(i);

        if (g->is<Part::GeomPoint>()) {
            const Base::Vector3d pos = static_cast<const Part::GeomPoint*>(g)->getPoint();
            perGeo[geoId].pointHandle = solver.addPoint(pos);
        }
        else if (g->is<Part::GeomLineSegment>()) {
            const auto* seg = static_cast<const Part::GeomLineSegment*>(g);
            GeoMapping m;
            m.startHandle = solver.addPoint(seg->getStartPoint());
            m.endHandle = solver.addPoint(seg->getEndPoint());
            m.lineHandle = solver.addLine(m.startHandle, m.endHandle);
            perGeo[geoId] = m;
        }
        // Circle / Arc / others deferred.
    }

    for (std::size_t ci = 0; ci < constraints.size(); ++ci) {
        const Constraint3D& c = constraints[ci];
        const int tagId = static_cast<int>(ci) + 1;  // GCS tag 0 is reserved.

        switch (c.Type) {
            case Constraint3D::Coincident3D: {
                if (c.getElements().size() < 2) {
                    break;
                }
                const int a = resolvePointHandle(c.getElements()[0]);
                const int b = resolvePointHandle(c.getElements()[1]);
                if (a >= 0 && b >= 0) {
                    solver.addConstraintCoincident(tagId, a, b);
                }
                break;
            }
            default:
                // Other constraint types lit up in later.
                break;
        }
    }
}

int GeometryMapper3D::resolvePointHandle(const GeoElementId3D& ref) const
{
    auto it = perGeo.find(ref.GeoId);
    if (it == perGeo.end()) {
        return -1;
    }
    const GeoMapping& m = it->second;

    switch (ref.Pos) {
        case PointPos::none:
            return m.pointHandle;  // Geometry is a standalone point.
        case PointPos::start:
            return m.startHandle;
        case PointPos::end:
            return m.endHandle;
        case PointPos::mid:
            return -1;  // Not yet meaningful for Point/Line.
    }
    return -1;
}

void GeometryMapper3D::writeBack(const Solver3D& solver, std::vector<Part::Geometry*>& geoList) const
{
    for (const auto& [geoId, m] : perGeo) {
        if (geoId < 0 || geoId >= static_cast<int>(geoList.size())) {
            continue;
        }
        Part::Geometry* g = geoList[geoId];
        if (!g) {
            continue;
        }

        if (m.pointHandle >= 0 && g->is<Part::GeomPoint>()) {
            static_cast<Part::GeomPoint*>(g)->setPoint(solver.getPoint(m.pointHandle));
        }
        else if (m.startHandle >= 0 && m.endHandle >= 0 && g->is<Part::GeomLineSegment>()) {
            auto* seg = static_cast<Part::GeomLineSegment*>(g);
            seg->setPoints(solver.getPoint(m.startHandle), solver.getPoint(m.endHandle));
        }
    }
}
