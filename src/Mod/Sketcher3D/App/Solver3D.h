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


#ifndef SKETCHER3D_SOLVER3D_H
#define SKETCHER3D_SOLVER3D_H

#include <memory>
#include <vector>

#include <Base/Vector3D.h>
#include <Mod/Sketcher/App/planegcs/GCS.h>
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

    /// Allocate an owned triple (x, y, z) of parameters and return a GCS
    /// Point3D referring to them.
    int addPoint(const Base::Vector3d& pos);

    /// Line between two previously added points.
    int addLine(int pointHandleA, int pointHandleB);

    /// Coincident constraint between two points.
    void addConstraintCoincident(int tagId, int pointHandleA, int pointHandleB);

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
    /// allocator for solver 
    double* allocParam(double value);

    GCS::System gcs;

    // pointers storage, for the duration of the solve.
    std::vector<std::unique_ptr<double>> ownedParams;

    // Mapping from handle index to underlying GCS struct.
    std::vector<GCS::Point3D> points;
    std::vector<GCS::Line3D> lines;

    // Flattened list of parameters fed to GCS::declareUnknowns.
    std::vector<double*> parameters;

    std::vector<int> conflictingTags;
    std::vector<int> redundantTags;
};

}  // namespace Sketcher3D

#endif  // SKETCHER3D_SOLVER3D_H
