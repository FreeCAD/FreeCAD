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

#include <Base/Console.h>
#include <Mod/Part/App/Geometry.h>

#include "Constraint3D.h"
#include "GeometryMapper3D.h"
#include "Solver3D.h"


using namespace Sketcher3D;

GeometryMapper3D::GeometryMapper3D() = default;

GeometryMapper3D::~GeometryMapper3D() = default;

void GeometryMapper3D::setUpSketch(
    const std::vector<Part::Geometry*>& geoList,
    const std::vector<Constraint3D>& constraints,
    Solver3D& solver
)
{
    clear(solver);
    rootPointId = solver.addPoint(Base::Vector3d(0.0, 0.0, 0.0), true);
    addGeometry(geoList, solver);
    addConstraints(constraints, solver);
}

void GeometryMapper3D::clear(Solver3D& solver)
{
    solver.clear();
    Geoms.clear();
    rootPointId = -1;
    MalformedConstraints.clear();
}

void GeometryMapper3D::addGeometry(const std::vector<Part::Geometry*>& geoList, Solver3D& solver)
{
    Geoms.resize(geoList.size());

    for (std::size_t i = 0; i < geoList.size(); ++i) {
        const Part::Geometry* geometry = geoList[i];
        if (!geometry) {
            continue;
        }

        GeoDef& def = Geoms[i];
        def.geo.reset(geometry->clone());

        if (def.geo->is<Part::GeomPoint>()) {
            const auto* point = static_cast<const Part::GeomPoint*>(def.geo.get());
            def.type = Point;
            def.startPointId = solver.addPoint(point->getPoint());
        }
        else if (def.geo->is<Part::GeomLineSegment>()) {
            const auto* segment = static_cast<const Part::GeomLineSegment*>(def.geo.get());
            def.type = Line;
            def.startPointId = solver.addPoint(segment->getStartPoint());
            def.endPointId = solver.addPoint(segment->getEndPoint());
            def.index = solver.addLine(def.startPointId, def.endPointId);
        }
        // Circle / Arc / others deferred.
    }
}

int GeometryMapper3D::addConstraints(const std::vector<Constraint3D>& constraints, Solver3D& solver)
{
    int result = -1;

    for (std::size_t i = 0; i < constraints.size(); ++i) {
        int tagId = static_cast<int>(i) + 1;
        result = addConstraint(constraints[i], tagId, solver);
        if (result < 0) {
            Base::Console().error("Sketcher3D constraint number %d is malformed!\n", tagId);
            MalformedConstraints.push_back(tagId);
        }
    }

    return result;
}

int GeometryMapper3D::addConstraint(const Constraint3D& constraint, int tagId, Solver3D& solver)
{
    const auto& elements = constraint.getElements();

    switch (constraint.Type) {
        case Constraint3D::Distance3D: {
            if (elements.size() != 2) {
                return -1;
            }
            const int a = getPointId(elements[0]);
            const int b = getPointId(elements[1]);
            if (a < 0 || b < 0) {
                return -1;
            }
            solver.addConstraintDistance(tagId, a, b, constraint.Value);
            return tagId;
        }
        case Constraint3D::DistanceX3D:
        case Constraint3D::DistanceY3D:
        case Constraint3D::DistanceZ3D: {
            if (elements.empty() || elements.size() > 2) {
                return -1;
            }

            const int a = getPointId(elements[0]);
            if (a < 0) {
                return -1;
            }

            if (elements.size() == 1) {
                if (constraint.Type == Constraint3D::DistanceX3D) {
                    solver.addConstraintCoordinateX(tagId, a, constraint.Value);
                }
                else if (constraint.Type == Constraint3D::DistanceY3D) {
                    solver.addConstraintCoordinateY(tagId, a, constraint.Value);
                }
                else {
                    solver.addConstraintCoordinateZ(tagId, a, constraint.Value);
                }
                return tagId;
            }

            const int b = getPointId(elements[1]);
            if (b < 0) {
                return -1;
            }
            if (constraint.Type == Constraint3D::DistanceX3D) {
                solver.addConstraintDistanceX(tagId, a, b, constraint.Value);
            }
            else if (constraint.Type == Constraint3D::DistanceY3D) {
                solver.addConstraintDistanceY(tagId, a, b, constraint.Value);
            }
            else {
                solver.addConstraintDistanceZ(tagId, a, b, constraint.Value);
            }
            return tagId;
        }
        case Constraint3D::Coincident3D: {
            if (elements.size() != 2) {
                return -1;
            }
            const int a = getPointId(elements[0]);
            const int b = getPointId(elements[1]);
            if (a < 0 || b < 0) {
                return -1;
            }
            solver.addConstraintCoincident(tagId, a, b);
            return tagId;
        }
        case Constraint3D::Parallel3D: {
            if (elements.size() != 2 || elements[0].Pos != PointPos::none
                || elements[1].Pos != PointPos::none) {
                return -1;
            }
            const int a = getLineId(elements[0]);
            const int b = getLineId(elements[1]);
            if (a < 0 || b < 0) {
                return -1;
            }
            solver.addConstraintParallel(tagId, a, b);
            return tagId;
        }
        case Constraint3D::EqualLength3D: {
            if (elements.size() != 2 || elements[0].Pos != PointPos::none
                || elements[1].Pos != PointPos::none) {
                return -1;
            }
            const int a = getLineId(elements[0]);
            const int b = getLineId(elements[1]);
            if (a < 0 || b < 0) {
                return -1;
            }
            solver.addConstraintEqualLength(tagId, a, b);
            return tagId;
        }
        case Constraint3D::Angle3D: {
            if (elements.size() != 2) {
                return -1;
            }
            const auto validDirection = [](PointPos pos) {
                return pos == PointPos::none || pos == PointPos::start || pos == PointPos::end;
            };
            if (!validDirection(elements[0].Pos) || !validDirection(elements[1].Pos)) {
                return -1;
            }

            const int a = getLineId(elements[0]);
            const int b = getLineId(elements[1]);
            if (a < 0 || b < 0) {
                return -1;
            }
            solver.addConstraintAngle(tagId, a, elements[0].Pos, b, elements[1].Pos, constraint.Value);
            return tagId;
        }
        case Constraint3D::AlongX:
        case Constraint3D::AlongY:
        case Constraint3D::AlongZ: {
            if (elements.size() != 1 || elements[0].Pos != PointPos::none) {
                return -1;
            }
            const int line = getLineId(elements[0]);
            if (line < 0) {
                return -1;
            }
            if (constraint.Type == Constraint3D::AlongX) {
                solver.addConstraintAlongX(tagId, line);
            }
            else if (constraint.Type == Constraint3D::AlongY) {
                solver.addConstraintAlongY(tagId, line);
            }
            else {
                solver.addConstraintAlongZ(tagId, line);
            }
            return tagId;
        }
        case Constraint3D::PointOnLine3D:
        case Constraint3D::PointAtLineMidpoint3D: {
            if (elements.size() != 2) {
                return -1;
            }
            int lineIdx = -1;
            int line = -1;
            for (int x = 0; x < 2; ++x) {
                if (elements[x].Pos == PointPos::none) {
                    const int candidate = getLineId(elements[x]);
                    if (candidate >= 0) {
                        lineIdx = x;
                        line = candidate;
                        break;
                    }
                }
            }
            if (lineIdx < 0) {
                return -1;
            }
            const int point = getPointId(elements[1 - lineIdx]);
            if (point < 0) {
                return -1;
            }
            if (constraint.Type == Constraint3D::PointOnLine3D) {
                solver.addConstraintPointOnLine(tagId, point, line);
            }
            else {
                solver.addConstraintPointAtLineMidpoint(tagId, point, line);
            }
            return tagId;
        }
        case Constraint3D::Collinear3D: {
            if (elements.size() != 2 || elements[0].Pos != PointPos::none
                || elements[1].Pos != PointPos::none) {
                return -1;
            }
            const int a = getLineId(elements[0]);
            const int b = getLineId(elements[1]);
            if (a < 0 || b < 0) {
                return -1;
            }
            solver.addConstraintCollinear(tagId, a, b);
            return tagId;
        }
        default:
            return -1;
    }
}

int GeometryMapper3D::getPointId(const GeoElementId3D& ref) const
{
    if (ref == GeoElementId3D::RtPnt) {
        return rootPointId;
    }

    if (!ref.isValid() || ref.GeoId < 0 || ref.GeoId >= static_cast<int>(Geoms.size())) {
        return -1;
    }

    const GeoDef& def = Geoms[ref.GeoId];
    switch (ref.Pos) {
        case PointPos::none:
            return def.type == Point ? def.startPointId : -1;
        case PointPos::start:
            return def.startPointId;
        case PointPos::end:
            return def.endPointId;
        case PointPos::mid:
            return def.midPointId;
    }
    return -1;
}

int GeometryMapper3D::getLineId(const GeoElementId3D& ref) const
{
    if (!ref.isValid() || ref.GeoId < 0 || ref.GeoId >= static_cast<int>(Geoms.size())) {
        return -1;
    }

    const GeoDef& def = Geoms[ref.GeoId];
    return def.type == Line ? def.index : -1;
}

void GeometryMapper3D::updateGeometry(const Solver3D& solver)
{
    for (GeoDef& def : Geoms) {
        if (!def.geo) {
            continue;
        }

        if (def.type == Point && def.startPointId >= 0) {
            auto* point = static_cast<Part::GeomPoint*>(def.geo.get());
            point->setPoint(solver.getPoint(def.startPointId));
        }
        else if (def.type == Line && def.startPointId >= 0 && def.endPointId >= 0) {
            auto* segment = static_cast<Part::GeomLineSegment*>(def.geo.get());
            segment->setPoints(solver.getPoint(def.startPointId), solver.getPoint(def.endPointId));
        }
    }
}

std::vector<Part::Geometry*> GeometryMapper3D::extractGeometry() const
{
    std::vector<Part::Geometry*> geometry;
    geometry.reserve(Geoms.size());

    for (const GeoDef& def : Geoms) {
        geometry.push_back(def.geo ? def.geo->clone() : nullptr);
    }

    return geometry;
}
