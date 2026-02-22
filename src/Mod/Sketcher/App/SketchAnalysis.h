// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2018 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Precision.hxx>

#include <Base/Vector3D.h>

#include "Analyse.h"


namespace Sketcher
{

class SketchObject;


enum class Solver
{
    RedundantConstraints = -2,
    ConflictingConstraints = -3,
    OverConstrained = -4,
};


class SketcherExport SketchAnalysis
{
public:
    /// Creates an instance of the SketchAnalysis object, taking as parameter a pointer to an
    /// SketchObject.
    ///
    /// There is a first type of routines, simple routines, which work in the following order:
    /// Detect - (Analyse) - [Get] - [Set] - Make
    ///
    /// The Detect step just identifies possible missing constraints.
    ///
    /// The Analyse, which is not available for all the routines, operates in detected constraints
    /// of the same routine, to look for alternatives. For example, a general pointonpoint detection
    /// leads to a search for coincident constraints, which can be later run via Analyse if it is
    /// intended to convert endpoint coincidence to endpoint perpendicular and tangent constraints.
    ///
    /// The Get retrieves the result of the analysis as a vector of ConstraintIds, indicating the
    /// suggested constraints. This step is intended for enabling the user to check the result of
    /// the analysis, rather than applying it. If only applying is intended, this step is not
    /// necessary as the Make will operate on the result of the Detect - Analyse directly.
    ///
    /// The Set changes the detected result. It modifies the SketchAnalysis object. It only modifies
    /// the SketchObject as far as the SketchAnalysis is changed. It does not apply any changes to
    /// the sketch. It is intended so as to enable the user to change the result that will be
    /// applied.
    ///
    /// Neither the Detect, nor the Analyse, nor the Get steps modify the Sketch geometry.
    ///
    /// Make applies the constraints stored internally in the SketchAnalysis object.
    ///
    /// A second type of routines, complex routines, are thought for running fully automatic and
    /// they Detect, Analyse and Make. They may also apply a variety of constraint types.
    ///
    /// A third type of routines do not relate to autoconstraining at all, and include validation
    /// methods for sketches.
    explicit SketchAnalysis(Sketcher::SketchObject* Obj);
    ~SketchAnalysis();

    // Simple routines (see constructor)

    /// Point on Point constraint simple routine Detect step (see constructor)
    /// Detect detects only coincident constraints, Analyse converts coincident to endpoint
    /// perpendicular/tangent where appropriate
    int detectMissingPointOnPointConstraints(
        double precision = Precision::Confusion() * 1000,
        bool includeconstruction = true
    );
    /// Point on Point constraint simple routine Analyse step (see constructor)
    void analyseMissingPointOnPointCoincident(double angleprecision = std::numbers::pi / 8);
    /// Point on Point constraint simple routine Get step (see constructor)
    std::vector<ConstraintIds>& getMissingPointOnPointConstraints()
    {
        return vertexConstraints;
    }
    /// Vertical/Horizontal constraints simple routine Set step (see constructor)
    void setMissingPointOnPointConstraints(std::vector<ConstraintIds>& cl)
    {
        vertexConstraints = cl;
    }
    /// Point on Point constraint simple routine Make step (see constructor)
    void makeMissingPointOnPointCoincident();
    /// Point on Point constraint simple routine Make step (see constructor)
    /// The sketch is solved after each individual constraint addition and any
    /// redundancy removed.
    void makeMissingPointOnPointCoincidentOneByOne();

    /// Vertical/Horizontal constraints simple routine Detect step (see constructor)
    int detectMissingVerticalHorizontalConstraints(double angleprecision = std::numbers::pi / 8);
    /// Vertical/Horizontal constraints simple routine Get step (see constructor)
    std::vector<ConstraintIds>& getMissingVerticalHorizontalConstraints()
    {
        return verthorizConstraints;
    }
    /// Vertical/Horizontal constraints simple routine Set step (see constructor)
    void setMissingVerticalHorizontalConstraints(std::vector<ConstraintIds>& cl)
    {
        verthorizConstraints = cl;
    }
    /// Vertical/Horizontal constraints simple routine Make step (see constructor)
    void makeMissingVerticalHorizontal();
    void makeMissingVerticalHorizontalOneByOne();

    /// Equality constraints simple routine Detect step (see constructor)
    int detectMissingEqualityConstraints(double precision);
    /// Equality constraints simple routine Get step for line segments (see constructor)
    std::vector<ConstraintIds>& getMissingLineEqualityConstraints()
    {
        return lineequalityConstraints;
    }
    /// Equality constraints simple routine Get step for radii (see constructor)
    std::vector<ConstraintIds>& getMissingRadiusConstraints()
    {
        return radiusequalityConstraints;
    }
    /// Equality constraints simple routine Set step for line segments (see constructor)
    void setMissingLineEqualityConstraints(std::vector<ConstraintIds>& cl)
    {
        lineequalityConstraints = cl;
    }
    /// Equality constraints simple routine Set step for radii (see constructor)
    void setMissingRadiusConstraints(std::vector<ConstraintIds>& cl)
    {
        radiusequalityConstraints = cl;
    }
    /// Equality constraints simple routine Make step (see constructor)
    void makeMissingEquality();
    void makeMissingEqualityOneByOne();

    /// Detect degenerated geometries
    int detectDegeneratedGeometries(double tolerance) const;
    /// Remove degenerated geometries
    int removeDegeneratedGeometries(double tolerance);

    // Complex routines (see constructor)

    /// Fully automated multi-constraint autoconstraining
    ///
    /// It DELETES all the constraints currently present in the Sketcher. The reason is that it
    /// makes assumptions to avoid redundancies.
    ///
    /// It applies coincidents - vertical/horizontal constraints and equality constraints.
    int autoconstraint(
        double precision = Precision::Confusion() * 1000,
        double angleprecision = std::numbers::pi / 8,
        bool includeconstruction = true
    );

    // helper functions, which may be used by more complex methods, and/or called directly by user
    // space (python) methods

    /// solves the sketch and retrieves the error status, and the degrees of freedom.
    /// It enables to solve updating the geometry (so moving the geometry to match the constraints)
    /// or preserving the geometry.
    void solvesketch(int& status, int& dofs, bool updategeo);

    // third type of routines
    std::vector<Base::Vector3d> getOpenVertices() const;

private:
    Sketcher::SketchObject* sketch;

    std::vector<ConstraintIds> vertexConstraints;
    std::vector<ConstraintIds> verthorizConstraints;
    std::vector<ConstraintIds> lineequalityConstraints;
    std::vector<ConstraintIds> radiusequalityConstraints;

private:
    void autoDeleteAllConstraints();
    void autoHorizontalVerticalConstraints();
    void autoPointOnPointCoincident();
    void autoMissingEquality();
    bool checkHorizontal(Base::Vector3d dir, double angleprecision);
    bool checkVertical(Base::Vector3d dir, double angleprecision);
    void makeConstraints(std::vector<ConstraintIds>&);
    void makeConstraintsOneByOne(std::vector<ConstraintIds>&, const char* errorText);
    std::set<int> getDegeneratedGeometries(double tolerance) const;
    void solveSketch(const char* errorText);
    static Sketcher::Constraint* create(const ConstraintIds& id);
};

}  // namespace Sketcher
