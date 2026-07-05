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

#include <memory>
#include <vector>

#include <Base/Vector3D.h>
#include <Mod/Sketcher/App/planegcs/GCS.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>
#include <Mod/Sketcher3D/Sketcher3DGlobal.h>


namespace Sketcher3D
{


class Sketcher3DExport Solver3D
{
public:
    /// Solver return codes.
    enum Status : int
    {
        OK = 0,
        SolverFailed = -1,
        Redundant = -2,
        Conflicting = -3,
        OverConstrained = -4,
        Malformed = -5,
    };

    Solver3D();
    ~Solver3D();

    Solver3D(const Solver3D&) = delete;
    Solver3D& operator=(const Solver3D&) = delete;

    /// Drop all geometry, constraints, and parameters.
    void clear();

    /// Add a point, optionally fixed parameters.
    int addPoint(const Base::Vector3d& pos, bool fixed = false);

    /// Line between two previously added points.
    int addLine(int pointHandleA, int pointHandleB);

    /// Fixed reference plane.
    int addPlane(const Base::Vector3d& origin, const Base::Vector3d& normal);

    /// Coincident constraint between two points.
    void addConstraintCoincident(int tagId, int pointHandleA, int pointHandleB);

    /// Parallel constraint between two lines.
    void addConstraintParallel(int tagId, int lineHandleA, int lineHandleB);

    /// Equal length constraint between two lines.
    void addConstraintEqualLength(int tagId, int lineHandleA, int lineHandleB);

    /// Pointonline constraint
    void addConstraintPointOnLine(int tagId, int pointHandle, int lineHandle);

    /// Point at the midpoint of a line segment.
    void addConstraintPointAtLineMidpoint(int tagId, int pointHandle, int lineHandle);

    /// Collinear constraint between two lines.
    void addConstraintCollinear(int tagId, int lineHandleA, int lineHandleB);

    /// Angle constraint between two lines.
    /// Angle is unsigned, in radians, in [0, pi].
    void addConstraintAngle(
        int tagId,
        int lineHandleA,
        PointPos posA,
        int lineHandleB,
        PointPos posB,
        double angle
    );

    /// Constraint to align a line along the X axis.
    void addConstraintAlongX(int tagId, int lineHandle);

    /// Constraint to align a line along the Y axis.
    void addConstraintAlongY(int tagId, int lineHandle);

    /// Constraint to align a line along the Z axis.
    void addConstraintAlongZ(int tagId, int lineHandle);

    /// P2P distance constraint.
    void addConstraintDistance(int tagId, int pointHandleA, int pointHandleB, double distance);

    /// P2L distance constraint.
    void addConstraintDistancePointToLine(int tagId, int pointHandle, int lineHandle, double distance);

    /// Signed X-distance constraint between two points.
    void addConstraintDistanceX(int tagId, int pointHandleA, int pointHandleB, double distance);

    /// Signed Y-distance constraint between two points.
    void addConstraintDistanceY(int tagId, int pointHandleA, int pointHandleB, double distance);

    /// Signed Z-distance constraint between two points.
    void addConstraintDistanceZ(int tagId, int pointHandleA, int pointHandleB, double distance);

    /// Lock the X coordinate of a single point.
    void addConstraintCoordinateX(int tagId, int pointHandle, double value);

    /// Lock the Y coordinate of a single point.
    void addConstraintCoordinateY(int tagId, int pointHandle, double value);

    /// Lock the Z coordinate of a single point.
    void addConstraintCoordinateZ(int tagId, int pointHandle, double value);

    /// Project a point onto a fixed plane.
    void addConstraintProjectOnPlane(int tagId, int pointHandle, int planeHandle);

    int solve();

    /// Read back the current value of a point after solve().
    Base::Vector3d getPoint(int pointHandle) const;

    bool hasConflicts() const
    {
        return !conflictingTags.empty();
    }
    const std::vector<int>& getConflicting() const
    {
        return conflictingTags;
    }
    bool hasRedundancies() const
    {
        return !redundantTags.empty();
    }
    const std::vector<int>& getRedundant() const
    {
        return redundantTags;
    }

private:
    double* allocParam(double value);
    double* allocFixParam(double value);

    GCS::System GCSsys;

    // storage for parameters, owned by the solver.
    std::vector<std::unique_ptr<double>> parameterStorage;
    std::vector<std::unique_ptr<double>> fixedParameterStorage;

    // Mapping from handle index to underlying GCS struct.
    std::vector<GCS::Point3D> points;
    std::vector<GCS::Line3D> lines;

    // TODO: plane is not a Dof in solver.
    struct Plane3D
    {
        Base::Vector3d origin;
        Base::Vector3d normal;
    };
    std::vector<Plane3D> planes;

    std::vector<double*> parameters;

    std::vector<int> conflictingTags;
    std::vector<int> redundantTags;
};

}  // namespace Sketcher3D
