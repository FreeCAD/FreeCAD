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
#include <Geom_Circle.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include "Constraint3D.h"
#include "GeomReferencePlane3D.h"
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
        Part::Geometry* geometry = geoList[i];
        if (!geometry) {
            continue;
        }

        GeoDef& def = Geoms[i];
        def.geo.reset(geometry->clone());
        def.type = kindOfGeometry(def.geo.get());

        switch (def.type) {
            case GeoKind::Point: {
                auto* point = static_cast<Part::GeomPoint*>(def.geo.get());
                def.startPointId = solver.addPoint(point->getPoint());
                break;
            }
            case GeoKind::Line: {
                auto* segment = static_cast<Part::GeomLineSegment*>(def.geo.get());
                def.startPointId = solver.addPoint(segment->getStartPoint());
                def.endPointId = solver.addPoint(segment->getEndPoint());
                def.index = solver.addLine(def.startPointId, def.endPointId);
                break;
            }
            case GeoKind::Plane: {
                auto* plane = static_cast<GeomReferencePlane3D*>(def.geo.get());
                def.index = solver.addPlane(plane->getLocation(), plane->getDir());
                break;
            }
            case GeoKind::Arc: {
                auto* arc = static_cast<Part::GeomArcOfCircle*>(def.geo.get());
                def.midPointId = solver.addPoint(arc->getCenter());
                def.startPointId = solver.addPoint(arc->getStartPoint());
                def.endPointId = solver.addPoint(arc->getEndPoint());

                double startAngle = 0.0;
                double endAngle = 0.0;
                arc->getRange(startAngle, endAngle, /*emulateCCW=*/false);

                def.index = solver.addArc(
                    def.midPointId,
                    def.startPointId,
                    def.endPointId,
                    arc->getRadius(),
                    startAngle,
                    endAngle,
                    arc->getAxisDirection(),
                    arc->getXAxisDir()
                );
                solver.addConstraintArcRules(0, def.index);
                break;
            }
            case GeoKind::Circle: {
                auto* circle = static_cast<Part::GeomCircle*>(def.geo.get());
                def.midPointId = solver.addPoint(circle->getCenter());

                Base::Vector3d xDir(1.0, 0.0, 0.0);
                Handle(Geom_Circle) gc = Handle(Geom_Circle)::DownCast(circle->handle());
                if (!gc.IsNull()) {
                    gp_Dir xd = gc->Position().XDirection();
                    xDir.Set(xd.X(), xd.Y(), xd.Z());
                }

                def.index = solver.addCircle(
                    def.midPointId,
                    circle->getRadius(),
                    circle->getAxisDirection(),
                    xDir
                );
                break;
            }
            default:
                break;
        }
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

            // P2P
            int a = getPointId(elements[0]);
            int b = getPointId(elements[1]);
            if (a >= 0 && b >= 0) {
                solver.addConstraintDistance(tagId, a, b, constraint.Value);
                return tagId;
            }

            // P2L
            int lineId = -1;
            int pointId = -1;
            if (a >= 0 && (lineId = getLineId(elements[1])) >= 0) {
                pointId = a;
            }
            else if (b >= 0 && (lineId = getLineId(elements[0])) >= 0) {
                pointId = b;
            }

            if (pointId >= 0 && lineId >= 0) {
                solver.addConstraintDistancePointToLine(tagId, pointId, lineId, constraint.Value);
                return tagId;
            }

            return -1;
        }
        case Constraint3D::DistanceX3D:
        case Constraint3D::DistanceY3D:
        case Constraint3D::DistanceZ3D: {
            if (elements.empty() || elements.size() > 2) {
                return -1;
            }

            int a = getPointId(elements[0]);
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

            int b = getPointId(elements[1]);
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
            int a = getPointId(elements[0]);
            int b = getPointId(elements[1]);
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
            int a = getLineId(elements[0]);
            int b = getLineId(elements[1]);
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
            int a = getLineId(elements[0]);
            int b = getLineId(elements[1]);
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
            auto validDirection = [](PointPos pos) {
                return pos == PointPos::none || pos == PointPos::start || pos == PointPos::end;
            };
            if (!validDirection(elements[0].Pos) || !validDirection(elements[1].Pos)) {
                return -1;
            }

            int a = getLineId(elements[0]);
            int b = getLineId(elements[1]);
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
            int line = getLineId(elements[0]);
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
        case Constraint3D::PointOnCurve3D: {
            if (elements.size() != 2) {
                return -1;
            }
            int pointIdx = -1;
            int curveIdx = -1;
            for (int x = 0; x < 2; ++x) {
                if (elements[x].Pos != PointPos::none) {
                    pointIdx = x;
                }
                else {
                    curveIdx = x;
                }
            }
            if (pointIdx < 0 || curveIdx < 0) {
                return -1;
            }
            int point = getPointId(elements[pointIdx]);
            if (point < 0) {
                return -1;
            }
            GeoDef& curveDef = Geoms[elements[curveIdx].GeoId];
            switch (curveDef.type) {
                case GeoKind::Line: {
                    int line = getLineId(elements[curveIdx]);
                    if (line < 0) {
                        return -1;
                    }
                    solver.addConstraintPointOnLine(tagId, point, line);
                    break;
                }
                case GeoKind::Arc:
                    solver.addConstraintPointOnArc(tagId, point, curveDef.index);
                    break;
                case GeoKind::Circle:
                    solver.addConstraintPointOnCircle(tagId, point, curveDef.index);
                    break;
                default:
                    return -1;
            }
            return tagId;
        }
        case Constraint3D::PointAtLineMidpoint3D: {
            if (elements.size() != 2) {
                return -1;
            }
            int pointIdx = -1;
            int curveIdx = -1;
            for (int x = 0; x < 2; ++x) {
                if (elements[x].Pos != PointPos::none) {
                    pointIdx = x;
                }
                else {
                    curveIdx = x;
                }
            }
            if (pointIdx < 0 || curveIdx < 0) {
                return -1;
            }
            int point = getPointId(elements[pointIdx]);
            if (point < 0) {
                return -1;
            }
            int line = getLineId(elements[curveIdx]);
            if (line < 0) {
                return -1;
            }
            solver.addConstraintPointAtLineMidpoint(tagId, point, line);
            return tagId;
        }
        case Constraint3D::Collinear3D: {
            if (elements.size() != 2 || elements[0].Pos != PointPos::none
                || elements[1].Pos != PointPos::none) {
                return -1;
            }
            int a = getLineId(elements[0]);
            int b = getLineId(elements[1]);
            if (a < 0 || b < 0) {
                return -1;
            }
            solver.addConstraintCollinear(tagId, a, b);
            return tagId;
        }
        case Constraint3D::Radius3D: {
            if (elements.size() != 1) {
                return -1;
            }
            GeoDef& def = Geoms[elements[0].GeoId];
            switch (def.type) {
                case GeoKind::Circle:
                    solver.addConstraintCircleRadius(tagId, def.index, constraint.Value);
                    return tagId;
                case GeoKind::Arc:
                    solver.addConstraintArcRadius(tagId, def.index, constraint.Value);
                    return tagId;
                default:
                    return -1;
            }
        }
        case Constraint3D::ProjectOnPlane3D: {
            if (elements.size() != 2) {
                return -1;
            }
            int plane = getPlaneId(elements[1]);
            if (plane < 0) {
                return -1;
            }

            // Point
            int point = getPointId(elements[0]);
            if (point >= 0) {
                solver.addConstraintProjectOnPlane(tagId, point, plane);
                return tagId;
            }

            // Line
            if (elements[0].Pos == PointPos::none && getLineId(elements[0]) >= 0) {
                GeoDef& def = Geoms[elements[0].GeoId];
                solver.addConstraintProjectOnPlane(tagId, def.startPointId, plane);
                solver.addConstraintProjectOnPlane(tagId, def.endPointId, plane);
                return tagId;
            }

            return -1;
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

    if (ref.GeoId < 0 || ref.GeoId >= static_cast<int>(Geoms.size())) {
        return -1;
    }

    const GeoDef& def = Geoms[ref.GeoId];
    switch (ref.Pos) {
        case PointPos::none:
            return def.type == GeoKind::Point ? def.startPointId : -1;
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
    if (ref.GeoId < 0 || ref.GeoId >= static_cast<int>(Geoms.size())) {
        return -1;
    }

    const GeoDef& def = Geoms[ref.GeoId];
    return def.type == GeoKind::Line ? def.index : -1;
}

int GeometryMapper3D::getPlaneId(const GeoElementId3D& ref) const
{
    if (ref.GeoId < 0 || ref.GeoId >= static_cast<int>(Geoms.size())) {
        return -1;
    }
    const GeoDef& def = Geoms[ref.GeoId];
    return def.type == GeoKind::Plane ? def.index : -1;
}

void GeometryMapper3D::updateGeometry(const Solver3D& solver)
{
    for (GeoDef& def : Geoms) {
        if (!def.geo) {
            continue;
        }

        switch (def.type) {
            case GeoKind::Point:
                if (def.startPointId >= 0) {
                    auto* point = static_cast<Part::GeomPoint*>(def.geo.get());
                    point->setPoint(solver.getPoint(def.startPointId));
                }
                break;
            case GeoKind::Line:
                if (def.startPointId >= 0 && def.endPointId >= 0) {
                    auto* segment = static_cast<Part::GeomLineSegment*>(def.geo.get());
                    segment->setPoints(
                        solver.getPoint(def.startPointId),
                        solver.getPoint(def.endPointId)
                    );
                }
                break;
            case GeoKind::Arc:
                if (def.index >= 0 && def.midPointId >= 0) {
                    auto* arc = static_cast<Part::GeomArcOfCircle*>(def.geo.get());
                    const Base::Vector3d center = solver.getPoint(def.midPointId);
                    const Solver3D::ArcFrame frame = solver.getArcFrame(def.index);

                    gp_Dir N(frame.normal.x, frame.normal.y, frame.normal.z);
                    gp_Dir X(frame.xAxis.x, frame.xAxis.y, frame.xAxis.z);
                    gp_Ax2 ax2(gp_Pnt(center.x, center.y, center.z), N, X);

                    Handle(Geom_Circle)
                        circ = new Geom_Circle(ax2, std::max(std::abs(frame.radius), 1e-7));
                    arc->setHandle(circ);
                    arc->setRange(frame.startAngle, frame.endAngle, false);
                }
                break;
            case GeoKind::Circle:
                if (def.index >= 0 && def.midPointId >= 0) {
                    auto* circle = static_cast<Part::GeomCircle*>(def.geo.get());
                    const Base::Vector3d center = solver.getPoint(def.midPointId);
                    const Solver3D::CircleFrame frame = solver.getCircleFrame(def.index);
                    Base::Vector3d xDirection = frame.xAxis
                        - frame.normal * frame.xAxis.Dot(frame.normal);
                    if (xDirection.Length() > 1e-12) {
                        xDirection.Normalize();
                    }

                    gp_Dir N(frame.normal.x, frame.normal.y, frame.normal.z);
                    gp_Dir X(xDirection.x, xDirection.y, xDirection.z);
                    gp_Ax2 ax2(gp_Pnt(center.x, center.y, center.z), N, X);

                    Handle(Geom_Circle)
                        circ = new Geom_Circle(ax2, std::max(std::abs(frame.radius), 1e-7));
                    circle->setHandle(circ);
                }
                break;
            default:
                break;
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
