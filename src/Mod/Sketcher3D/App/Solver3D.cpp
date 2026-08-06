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
#include <Base/Exception.h>

#include "GeoEnum3D.h"
#include "Solver3D.h"


using namespace Sketcher3D;

Solver3D::Solver3D() = default;

Solver3D::~Solver3D() = default;

void Solver3D::clear()
{
    GCSsys.clear();
    points.clear();
    lines.clear();
    planes.clear();
    circles.clear();
    arcs.clear();
    parameters.clear();
    parameterStorage.clear();
    fixedParameterStorage.clear();
    conflictingTags.clear();
    redundantTags.clear();
}

double* Solver3D::allocParam(double value)
{
    parameterStorage.emplace_back(std::make_unique<double>(value));
    parameters.push_back(parameterStorage.back().get());
    return parameterStorage.back().get();
}

double* Solver3D::allocFixParam(double value)
{
    fixedParameterStorage.emplace_back(std::make_unique<double>(value));
    return fixedParameterStorage.back().get();
}

int Solver3D::addPoint(const Base::Vector3d& pos, bool fixed)
{
    double* px = fixed ? allocFixParam(pos.x) : allocParam(pos.x);
    double* py = fixed ? allocFixParam(pos.y) : allocParam(pos.y);
    double* pz = fixed ? allocFixParam(pos.z) : allocParam(pos.z);
    points.emplace_back(px, py, pz);
    return static_cast<int>(points.size()) - 1;
}

int Solver3D::addLine(int pointHandleA, int pointHandleB)
{
    GCS::Line3D line;
    line.p1 = points[pointHandleA];
    line.p2 = points[pointHandleB];
    lines.emplace_back(line);
    return static_cast<int>(lines.size()) - 1;
}

// TODO: curently reference plane is fixed.
int Solver3D::addPlane(const Base::Vector3d& origin, const Base::Vector3d& normal)
{
    Base::Vector3d n = normal;
    if (n.Length() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Solver3D::addPlane normal is zero");
    }
    n.Normalize();

    Plane3D plane;
    plane.origin
        = GCS::Point3D(allocFixParam(origin.x), allocFixParam(origin.y), allocFixParam(origin.z));
    plane.normal = GCS::Point3D(allocFixParam(n.x), allocFixParam(n.y), allocFixParam(n.z));
    planes.push_back(plane);
    return static_cast<int>(planes.size()) - 1;
}

int Solver3D::addArc(
    int centerHandle,
    int startHandle,
    int endHandle,
    double radius,
    double startAngle,
    double endAngle,
    const Base::Vector3d& normal,
    const Base::Vector3d& xDirection
)
{
    GCS::Arc3D arc;
    arc.center = points[centerHandle];
    arc.start = points[startHandle];
    arc.end = points[endHandle];
    arc.rad = allocParam(radius);
    arc.startAngle = allocFixParam(0.0);
    arc.endAngle = allocParam(endAngle - startAngle);

    Base::Vector3d n = normal;
    n.Normalize();
    Base::Vector3d xAxisIn = xDirection - n * xDirection.Dot(n);
    xAxisIn.Normalize();
    Base::Vector3d yAxisIn = n.Cross(xAxisIn);
    Base::Vector3d xAxis = xAxisIn * std::cos(startAngle) + yAxisIn * std::sin(startAngle);

    arc.normal = GCS::Point3D(allocParam(n.x), allocParam(n.y), allocParam(n.z));
    arc.xAxis = GCS::Point3D(allocParam(xAxis.x), allocParam(xAxis.y), allocParam(xAxis.z));

    arcs.emplace_back(arc);

    return static_cast<int>(arcs.size()) - 1;
}

int Solver3D::addCircle(
    int centerHandle,
    double radius,
    const Base::Vector3d& normal,
    const Base::Vector3d& xDirection
)
{
    Base::Vector3d n = normal;
    n.Normalize();
    Base::Vector3d xAxis = xDirection - n * xDirection.Dot(n);
    xAxis.Normalize();

    GCS::Circle3D circ;
    circ.center = points[centerHandle];
    circ.rad = allocParam(radius);
    circ.normal = GCS::Point3D(allocParam(n.x), allocParam(n.y), allocParam(n.z));
    circ.xAxis = GCS::Point3D(allocFixParam(xAxis.x), allocFixParam(xAxis.y), allocFixParam(xAxis.z));

    circles.emplace_back(circ);

    return static_cast<int>(circles.size()) - 1;
}

void Solver3D::addConstraintArcRules(int tagId, int arcHandle)
{
    GCSsys.addConstraintArcRules3D(arcs[arcHandle], tagId, true);
}

void Solver3D::addConstraintCoincident(int tagId, int pointHandleA, int pointHandleB)
{
    GCSsys.addConstraintP2PCoincident3D(points[pointHandleA], points[pointHandleB], tagId);
}

void Solver3D::addConstraintParallel(int tagId, int lineHandleA, int lineHandleB)
{
    GCS::Line3D& la = lines[lineHandleA];
    GCS::Line3D& lb = lines[lineHandleB];
    GCSsys.addConstraintParallel3D(la.p1, la.p2, lb.p1, lb.p2, tagId);
}

void Solver3D::addConstraintEqualLength(int tagId, int lineHandleA, int lineHandleB)
{
    GCSsys.addConstraintEqualLength3D(lines[lineHandleA], lines[lineHandleB], tagId);
}

void Solver3D::addConstraintPointOnLine(int tagId, int pointHandle, int lineHandle)
{
    GCSsys.addConstraintPointOnLine3D(points[pointHandle], lines[lineHandle], tagId);
}

void Solver3D::addConstraintPointOnArc(int tagId, int pointHandle, int arcHandle)
{
    GCSsys.addConstraintPointOnCircle3D(points[pointHandle], arcs[arcHandle], tagId);
}

void Solver3D::addConstraintPointOnCircle(int tagId, int pointHandle, int circleHandle)
{
    GCSsys.addConstraintPointOnCircle3D(points[pointHandle], circles[circleHandle], tagId);
}

void Solver3D::addConstraintPointAtLineMidpoint(int tagId, int pointHandle, int lineHandle)
{
    GCSsys.addConstraintPointAtLineMidpoint3D(points[pointHandle], lines[lineHandle], tagId);
}

void Solver3D::addConstraintCollinear(int tagId, int lineHandleA, int lineHandleB)
{
    GCSsys.addConstraintCollinear3D(lines[lineHandleA], lines[lineHandleB], tagId);
}

void Solver3D::addConstraintAngle(
    int tagId,
    int lineHandleA,
    PointPos posA,
    int lineHandleB,
    PointPos posB,
    double angle
)
{
    double* a = allocFixParam(angle);
    GCS::Line3D& la = lines[lineHandleA];
    GCS::Line3D& lb = lines[lineHandleB];

    GCS::Point3D& la_tail = (posA == PointPos::end) ? la.p2 : la.p1;
    GCS::Point3D& la_head = (posA == PointPos::end) ? la.p1 : la.p2;
    GCS::Point3D& lb_tail = (posB == PointPos::end) ? lb.p2 : lb.p1;
    GCS::Point3D& lb_head = (posB == PointPos::end) ? lb.p1 : lb.p2;

    GCSsys.addConstraintL2LAngle3D(la_tail, la_head, lb_tail, lb_head, a, tagId);
}

void Solver3D::addConstraintAlongX(int tagId, int lineHandle)
{
    GCSsys.addConstraintLineAlongX3D(lines[lineHandle], tagId);
}

void Solver3D::addConstraintAlongY(int tagId, int lineHandle)
{
    GCSsys.addConstraintLineAlongY3D(lines[lineHandle], tagId);
}

void Solver3D::addConstraintAlongZ(int tagId, int lineHandle)
{
    GCSsys.addConstraintLineAlongZ3D(lines[lineHandle], tagId);
}

void Solver3D::addConstraintDistance(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    double* d = allocFixParam(distance);
    GCSsys.addConstraintP2PDistance3D(points[pointHandleA], points[pointHandleB], d, tagId);
}

void Solver3D::addConstraintDistancePointToLine(int tagId, int pointHandle, int lineHandle, double distance)
{
    double* d = allocFixParam(distance);
    GCSsys.addConstraintP2LDistance3D(points[pointHandle], lines[lineHandle], d, tagId);
}

void Solver3D::addConstraintDistanceX(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    double* d = allocFixParam(distance);
    GCSsys.addConstraintDifference(points[pointHandleA].x, points[pointHandleB].x, d, tagId);
}

void Solver3D::addConstraintDistanceY(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    double* d = allocFixParam(distance);
    GCSsys.addConstraintDifference(points[pointHandleA].y, points[pointHandleB].y, d, tagId);
}

void Solver3D::addConstraintDistanceZ(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    double* d = allocFixParam(distance);
    GCSsys.addConstraintDifference(points[pointHandleA].z, points[pointHandleB].z, d, tagId);
}

void Solver3D::addConstraintCoordinateX(int tagId, int pointHandle, double value)
{
    double* v = allocFixParam(value);
    GCSsys.addConstraintCoordinateX3D(points[pointHandle], v, tagId);
}

void Solver3D::addConstraintCoordinateY(int tagId, int pointHandle, double value)
{
    double* v = allocFixParam(value);
    GCSsys.addConstraintCoordinateY3D(points[pointHandle], v, tagId);
}

void Solver3D::addConstraintCoordinateZ(int tagId, int pointHandle, double value)
{
    double* v = allocFixParam(value);
    GCSsys.addConstraintCoordinateZ3D(points[pointHandle], v, tagId);
}

void Solver3D::addConstraintProjectOnPlane(int tagId, int pointHandle, int planeHandle)
{
    Plane3D& plane = planes[planeHandle];
    GCSsys.addConstraintProjectOnPlane3D(points[pointHandle], plane.origin, plane.normal, tagId);
}

void Solver3D::addConstraintCircleRadius(int tagId, int circleHandle, double radius)
{
    double* r = allocFixParam(radius);
    GCSsys.addConstraintCircleRadius3D(circles[circleHandle], r, tagId);
}

void Solver3D::addConstraintArcRadius(int tagId, int arcHandle, double radius)
{
    double* r = allocFixParam(radius);
    GCSsys.addConstraintCircleRadius3D(arcs[arcHandle], r, tagId);
}

int Solver3D::solve()
{
    conflictingTags.clear();
    redundantTags.clear();

    if (parameters.empty()) {
        return OK;
    }

    GCSsys.declareUnknowns(parameters);
    GCSsys.initSolution();

    GCSsys.diagnose();
    GCSsys.getConflicting(conflictingTags);
    GCSsys.getRedundant(redundantTags);

    int status = GCSsys.solve();
    if (status == GCS::Success || status == GCS::Converged) {
        GCSsys.applySolution();
        if (!redundantTags.empty()) {
            Base::Console().warning(
                "Sketcher3D debug: Solver3D solved but returns Redundant, redundantTags=%d\n",
                static_cast<int>(redundantTags.size())
            );
            return Redundant;
        }
        return OK;
    }
    if (status == GCS::SuccessfulSolutionInvalid) {
        return Malformed;
    }
    if (!conflictingTags.empty()) {
        return Conflicting;
    }
    return SolverFailed;
}

Base::Vector3d Solver3D::getPoint(int pointHandle) const
{
    const GCS::Point3D& p = points[pointHandle];
    return Base::Vector3d(*p.x, *p.y, *p.z);
}

Solver3D::CircleFrame Solver3D::getCircleFrame(int circleHandle) const
{
    const GCS::Circle3D& circle = circles[circleHandle];
    Base::Vector3d normal(*circle.normal.x, *circle.normal.y, *circle.normal.z);
    Base::Vector3d xAxis(*circle.xAxis.x, *circle.xAxis.y, *circle.xAxis.z);
    return {normal, xAxis, *circle.rad};
}

Solver3D::ArcFrame Solver3D::getArcFrame(int arcHandle) const
{
    const GCS::Arc3D& arc = arcs[arcHandle];
    Base::Vector3d normal(*arc.normal.x, *arc.normal.y, *arc.normal.z);
    Base::Vector3d xAxis(*arc.xAxis.x, *arc.xAxis.y, *arc.xAxis.z);
    return {normal, xAxis, *arc.rad, *arc.startAngle, *arc.endAngle};
}
