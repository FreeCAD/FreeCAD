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

#include <Base/Exception.h>

#include "Solver3D.h"


using namespace Sketcher3D;

Solver3D::Solver3D() = default;

Solver3D::~Solver3D() = default;

void Solver3D::clear()
{
    gcs.clear();
    ownedParams.clear();
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
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size())
        || pointHandleB < 0 || pointHandleB >= static_cast<int>(points.size())) {
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
    if (pointHandleA < 0 || pointHandleA >= static_cast<int>(points.size())
        || pointHandleB < 0 || pointHandleB >= static_cast<int>(points.size())) {
        throw Base::IndexError("Solver3D::addConstraintCoincident handle out of range");
    }
    gcs.addConstraintP2PCoincident3D(points[pointHandleA], points[pointHandleB], tagId);
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

    const int dofs = gcs.diagnose();
    (void)dofs;
    gcs.getConflicting(conflictingTags);
    gcs.getRedundant(redundantTags);

    if (!conflictingTags.empty()) {
        return dofs < 0 ? OverConstrained : Conflicting;
    }

    const int status = gcs.solve();
    if (status == GCS::Success || status == GCS::Converged) {
        gcs.applySolution();
        if (!redundantTags.empty()) {
            return Redundant;
        }
        return OK;
    }
    if (status == GCS::SuccessfulSolutionInvalid) {
        return Malformed;
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
