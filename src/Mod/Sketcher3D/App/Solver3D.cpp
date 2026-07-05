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

int Solver3D::addPlane(const Base::Vector3d& origin, const Base::Vector3d& normal)
{
    Base::Vector3d n = normal;
    if (n.Length() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Solver3D::addPlane normal is zero");
    }
    n.Normalize();

    Plane3D plane;
    plane.origin = origin;
    plane.normal = n;
    planes.push_back(plane);
    return static_cast<int>(planes.size()) - 1;
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
