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

#include <set>

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

    int firstPointHandle = -1;
    GeoElementId3D anchorRef;
    for (std::size_t i = 0; i < geoList.size(); ++i) {
        const Part::Geometry* g = geoList[i];
        if (!g) {
            continue;
        }
        const int geoId = static_cast<int>(i);

        if (g->is<Part::GeomPoint>()) {
            const Base::Vector3d pos = static_cast<const Part::GeomPoint*>(g)->getPoint();
            const int h = solver.addPoint(pos);
            perGeo[geoId].pointHandle = h;
            if (firstPointHandle < 0) {
                firstPointHandle = h;
                anchorRef = GeoElementId3D(geoId, PointPos::none, GeoKind::Point);
            }
        }
        else if (g->is<Part::GeomLineSegment>()) {
            const auto* seg = static_cast<const Part::GeomLineSegment*>(g);
            GeoMapping m;
            m.startHandle = solver.addPoint(seg->getStartPoint());
            m.endHandle = solver.addPoint(seg->getEndPoint());
            m.lineHandle = solver.addLine(m.startHandle, m.endHandle);
            perGeo[geoId] = m;
            if (firstPointHandle < 0) {
                firstPointHandle = m.startHandle;
                anchorRef = GeoElementId3D(geoId, PointPos::start, GeoKind::Line);
            }
        }
        // Circle / Arc / others deferred.
    }

    // Ground the first point to remove the 3 translational DOFs.
    // so this will anchor the sketch
    if (firstPointHandle >= 0) {
        // Build coincident equivalence class containing the anchor.
        std::set<GeoElementId3D> coincidentWithAnchor {anchorRef};
        std::map<GeoElementId3D, std::vector<GeoElementId3D>> coincidentEdges;
        for (const Constraint3D& c : constraints) {
            if (c.Type != Constraint3D::Coincident3D || c.getElements().size() < 2) {
                continue;
            }
            const auto& es = c.getElements();
            coincidentEdges[es[0]].push_back(es[1]);
            coincidentEdges[es[1]].push_back(es[0]);
        }
        std::vector<GeoElementId3D> pending {anchorRef};
        while (!pending.empty()) {
            const GeoElementId3D cur = pending.back();
            pending.pop_back();
            auto it = coincidentEdges.find(cur);
            if (it == coincidentEdges.end()) {
                continue;
            }
            for (const auto& nb : it->second) {
                if (coincidentWithAnchor.insert(nb).second) {
                    pending.push_back(nb);
                }
            }
        }

        bool lockX = true;
        bool lockY = true;
        bool lockZ = true;
        for (const Constraint3D& c : constraints) {
            if (c.getElements().size() != 1) {
                continue;
            }
            if (coincidentWithAnchor.find(c.getElements()[0]) == coincidentWithAnchor.end()) {
                continue;
            }
            if (c.Type == Constraint3D::DistanceX3D) {
                lockX = false;
            }
            else if (c.Type == Constraint3D::DistanceY3D) {
                lockY = false;
            }
            else if (c.Type == Constraint3D::DistanceZ3D) {
                lockZ = false;
            }
        }

        solver.groundPoint(firstPointHandle, 0, lockX, lockY, lockZ);
    }

    for (std::size_t ci = 0; ci < constraints.size(); ++ci) {
        const Constraint3D& c = constraints[ci];
        const int tagId = static_cast<int>(ci) + 1;  // GCS tag 0 is reserved.

        switch (c.Type) {
            case Constraint3D::Distance3D: {
                if (c.getElements().size() < 2) {
                    break;
                }
                const int a = resolvePointHandle(c.getElements()[0]);
                const int b = resolvePointHandle(c.getElements()[1]);
                if (a >= 0 && b >= 0) {
                    solver.addConstraintDistance(tagId, a, b, c.Value);
                }
                break;
            }
            case Constraint3D::DistanceX3D:
            case Constraint3D::DistanceY3D:
            case Constraint3D::DistanceZ3D: {
                if (c.getElements().empty()) {
                    break;
                }
                const int a = resolvePointHandle(c.getElements()[0]);
                if (a < 0) {
                    break;
                }
                // one selected element then take distance to origin along the axis.
                if (c.getElements().size() == 1) {
                    if (c.Type == Constraint3D::DistanceX3D) {
                        solver.addConstraintCoordinateX(tagId, a, c.Value);
                    }
                    else if (c.Type == Constraint3D::DistanceY3D) {
                        solver.addConstraintCoordinateY(tagId, a, c.Value);
                    }
                    else {
                        solver.addConstraintCoordinateZ(tagId, a, c.Value);
                    }
                    break;
                }
                // Two selected elements means distance between them.
                const int b = resolvePointHandle(c.getElements()[1]);
                if (b < 0) {
                    break;
                }
                if (c.Type == Constraint3D::DistanceX3D) {
                    solver.addConstraintDistanceX(tagId, a, b, c.Value);
                }
                else if (c.Type == Constraint3D::DistanceY3D) {
                    solver.addConstraintDistanceY(tagId, a, b, c.Value);
                }
                else {
                    solver.addConstraintDistanceZ(tagId, a, b, c.Value);
                }
                break;
            }
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
            case Constraint3D::Parallel3D: {
                if (c.getElements().size() < 2) {
                    break;
                }
                const int la = resolveLineHandle(c.getElements()[0]);
                const int lb = resolveLineHandle(c.getElements()[1]);
                if (la >= 0 && lb >= 0) {
                    solver.addConstraintParallel(tagId, la, lb);
                }
                break;
            }
            case Constraint3D::Angle3D: {
                if (c.getElements().size() < 2) {
                    break;
                }
                const int la = resolveLineHandle(c.getElements()[0]);
                const int lb = resolveLineHandle(c.getElements()[1]);
                if (la >= 0 && lb >= 0) {
                    solver.addConstraintAngle(
                        tagId,
                        la,
                        c.getElements()[0].Pos,
                        lb,
                        c.getElements()[1].Pos,
                        c.Value
                    );
                }
                break;
            }
            case Constraint3D::AlongX:
            case Constraint3D::AlongY:
            case Constraint3D::AlongZ: {
                if (c.getElements().size() != 1) {
                    break;
                }
                const int l = resolveLineHandle(c.getElements()[0]);
                if (l >= 0) {
                    if (c.Type == Constraint3D::AlongX) {
                        solver.addConstraintAlongX(tagId, l);
                    }
                    else if (c.Type == Constraint3D::AlongY) {
                        solver.addConstraintAlongY(tagId, l);
                    }
                    else if (c.Type == Constraint3D::AlongZ) {
                        solver.addConstraintAlongZ(tagId, l);
                    }
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
            return m.pointHandle;
        case PointPos::start:
            return m.startHandle;
        case PointPos::end:
            return m.endHandle;
        case PointPos::mid:
            return -1;
    }
    return -1;
}

int GeometryMapper3D::resolveLineHandle(const GeoElementId3D& ref) const
{
    auto it = perGeo.find(ref.GeoId);
    if (it == perGeo.end()) {
        return -1;
    }
    return it->second.lineHandle;
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
