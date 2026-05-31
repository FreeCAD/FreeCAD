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
    gcs.clear();
    ownedParams.clear();
    ownedDrivenParams.clear();
    points.clear();
    lines.clear();
    parameters.clear();
    conflictingTags.clear();
    redundantTags.clear();
}

double* Solver3D::allocParam(double value)
{
    ownedParams.emplace_back(std::make_unique<double>(value));
    parameters.push_back(ownedParams.back().get());
    return ownedParams.back().get();
}

double* Solver3D::allocDrivenParam(double value)
{
    ownedDrivenParams.emplace_back(std::make_unique<double>(value));
    return ownedDrivenParams.back().get();
}

int Solver3D::addPoint(const Base::Vector3d& pos)
{
    double* px = allocParam(pos.x);
    double* py = allocParam(pos.y);
    double* pz = allocParam(pos.z);
    points.emplace_back(px, py, pz);
    return static_cast<int>(points.size()) - 1;
}

int Solver3D::addLine(int pointHandleA, int pointHandleB)
{
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size()) || pointHandleB < 0
        || pointHandleB >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addLine point handle out of range");
    }
    GCS::Line3D line;
    line.p1 = points[pointHandleA];
    line.p2 = points[pointHandleB];
    lines.emplace_back(line);
    return static_cast<int>(lines.size()) - 1;
}

void Solver3D::addConstraintCoincident(int tagId, int pointHandleA, int pointHandleB)
{
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size()) || pointHandleB < 0
        || pointHandleB >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintCoincident handle out of range");
    }
    gcs.addConstraintP2PCoincident3D(points[pointHandleA], points[pointHandleB], tagId);
}

void Solver3D::addConstraintParallel(int tagId, int lineHandleA, int lineHandleB)
{
    if (lineHandleA < 0 || lineHandleA >= static_cast<int>(lines.size()) || lineHandleB < 0
        || lineHandleB >= static_cast<int>(lines.size())) {
        throw Base::IndexError("Solver3D::addConstraintParallel handle out of range");
    }
    GCS::Line3D& la = lines[lineHandleA];
    GCS::Line3D& lb = lines[lineHandleB];
    gcs.addConstraintParallel3D(la.p1, la.p2, lb.p1, lb.p2, tagId);
}

void Solver3D::addConstraintAngle(int tagId,
                                  int lineHandleA,
                                  PointPos posA,
                                  int lineHandleB,
                                  PointPos posB,
                                  double angle)
{
    if (lineHandleA < 0 || lineHandleA >= static_cast<int>(lines.size()) || lineHandleB < 0
        || lineHandleB >= static_cast<int>(lines.size())) {
        throw Base::IndexError("Solver3D::addConstraintAngle handle out of range");
    }
    double* a = allocDrivenParam(angle);
    GCS::Line3D& la = lines[lineHandleA];
    GCS::Line3D& lb = lines[lineHandleB];

    GCS::Point3D& la_tail = (posA == PointPos::end) ? la.p2 : la.p1;
    GCS::Point3D& la_head = (posA == PointPos::end) ? la.p1 : la.p2;
    GCS::Point3D& lb_tail = (posB == PointPos::end) ? lb.p2 : lb.p1;
    GCS::Point3D& lb_head = (posB == PointPos::end) ? lb.p1 : lb.p2;

    gcs.addConstraintL2LAngle3D(la_tail, la_head, lb_tail, lb_head, a, tagId);
}

void Solver3D::addConstraintAlongX(int tagId, int lineHandle)
{
    if (lineHandle < 0 || lineHandle >= static_cast<int>(lines.size())) {
        throw Base::IndexError("Solver3D::addConstraintAlongX handle out of range");
    }
    GCS::Line3D& line = lines[lineHandle];
    gcs.addConstraintLineAlongX3D(line, tagId);
}

void Solver3D::addConstraintAlongY(int tagId, int lineHandle)
{
    if (lineHandle < 0 || lineHandle >= static_cast<int>(lines.size())) {
        throw Base::IndexError("Solver3D::addConstraintAlongY handle out of range");
    }
    GCS::Line3D& line = lines[lineHandle];
    gcs.addConstraintLineAlongY3D(line, tagId);
}

void Solver3D::addConstraintAlongZ(int tagId, int lineHandle)
{
    if (lineHandle < 0 || lineHandle >= static_cast<int>(lines.size())) {
        throw Base::IndexError("Solver3D::addConstraintAlongZ handle out of range");
    }
    GCS::Line3D& line = lines[lineHandle];
    gcs.addConstraintLineAlongZ3D(line, tagId);
}

void Solver3D::addConstraintDistance(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size()) || pointHandleB < 0
        || pointHandleB >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintDistance handle out of range");
    }
    double* d = allocDrivenParam(distance);
    gcs.addConstraintP2PDistance3D(points[pointHandleA], points[pointHandleB], d, tagId);
}

void Solver3D::addConstraintDistanceX(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size()) || pointHandleB < 0
        || pointHandleB >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintDistanceX handle out of range");
    }
    double* d = allocDrivenParam(distance);
    GCS::Point3D& a = points[pointHandleA];
    GCS::Point3D& b = points[pointHandleB];
    gcs.addConstraintDifference(a.x, b.x, d, tagId);
}

void Solver3D::addConstraintDistanceY(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size()) || pointHandleB < 0
        || pointHandleB >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintDistanceY handle out of range");
    }
    double* d = allocDrivenParam(distance);
    GCS::Point3D& a = points[pointHandleA];
    GCS::Point3D& b = points[pointHandleB];
    gcs.addConstraintDifference(a.y, b.y, d, tagId);
}

void Solver3D::addConstraintDistanceZ(int tagId, int pointHandleA, int pointHandleB, double distance)
{
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size()) || pointHandleB < 0
        || pointHandleB >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintDistanceZ handle out of range");
    }
    double* d = allocDrivenParam(distance);
    GCS::Point3D& a = points[pointHandleA];
    GCS::Point3D& b = points[pointHandleB];
    gcs.addConstraintDifference(a.z, b.z, d, tagId);
}

void Solver3D::addConstraintCoordinateX(int tagId, int pointHandle, double value)
{
    if (pointHandle < 0 || pointHandle >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintCoordinateX handle out of range");
    }
    double* v = allocDrivenParam(value);
    gcs.addConstraintCoordinateX3D(points[pointHandle], v, tagId);
}

void Solver3D::addConstraintCoordinateY(int tagId, int pointHandle, double value)
{
    if (pointHandle < 0 || pointHandle >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintCoordinateY handle out of range");
    }
    double* v = allocDrivenParam(value);
    gcs.addConstraintCoordinateY3D(points[pointHandle], v, tagId);
}

void Solver3D::addConstraintCoordinateZ(int tagId, int pointHandle, double value)
{
    if (pointHandle < 0 || pointHandle >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintCoordinateZ handle out of range");
    }
    double* v = allocDrivenParam(value);
    gcs.addConstraintCoordinateZ3D(points[pointHandle], v, tagId);
}

void Solver3D::groundPoint(int pointHandle, int tagId, bool lockX, bool lockY, bool lockZ)
{
    if (pointHandle < 0 || pointHandle >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::groundPoint handle out of range");
    }
    GCS::Point3D& p = points[pointHandle];
    // Lock the current position on each requested axis.
    if (lockX) {
        double* gx = allocDrivenParam(*p.x);
        gcs.addConstraintCoordinateX3D(p, gx, tagId);
    }
    if (lockY) {
        double* gy = allocDrivenParam(*p.y);
        gcs.addConstraintCoordinateY3D(p, gy, tagId);
    }
    if (lockZ) {
        double* gz = allocDrivenParam(*p.z);
        gcs.addConstraintCoordinateZ3D(p, gz, tagId);
    }
}

int Solver3D::solve()
{
    conflictingTags.clear();
    redundantTags.clear();

    if (parameters.empty()) {
        return OK;
    }

    gcs.declareUnknowns(parameters);
    gcs.initSolution();

    gcs.diagnose();
    gcs.getConflicting(conflictingTags);
    gcs.getRedundant(redundantTags);

    const int status = gcs.solve();
    if (status == GCS::Success || status == GCS::Converged) {
        gcs.applySolution();
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
    if (pointHandle < 0 || pointHandle >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::getPoint handle out of range");
    }
    const GCS::Point3D& p = points[pointHandle];
    return Base::Vector3d(*p.x, *p.y, *p.z);
}
