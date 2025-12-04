
/***************************************************************************
 *   Copyright (c) 2025 Théo Veilleux-Trinh <theo.veilleux.trinh@proton.me>*
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

// The goal of this file is to handle substitutions, that is constraints which can be implicitly
// solved Currently the only substitution mecanism is reduction, so if two parameters are set to be
// equal (e.g. if a line is horizonal, y1=y2), then only one parameter y1 will be forwarded to the
// solver and all instances of y2 will be replaced by y1 in every constraints


#ifndef PLANEGCS_SUBSTITUTION_H
#define PLANEGCS_SUBSTITUTION_H

#include "Util.h"
#include "Constraints.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace GCS
{

/*
    Semantics used in this module:

    unknown: this is a double* that comes from outside the planegcs code, it is what the call site
   (usualy sketch.cpp) wants us to solve solver parameter: we can't just hand out the precious
   unknowns to the solver in case we mess them up, so we solve *parameters* instead.

   parameter: solver parameters are double that are copied from the unknowns, solved, and copied
   back if successful. The substitution class handles the lifetime of the solver parameters so
   subsystems get pointers to theses values within the vector
*/

struct SubstitutionUpdater
{
    double* root {nullptr};
    double* follower {nullptr};
    double offset {0};

    void apply() const
    {
        if (root == nullptr) {
            *follower = offset;
        }
        else {
            *follower = *root + offset;
        }
    }
};


/*
    The goal of the substitution class is to translate (non destructively) the set of constraints
   from the user into a more stable one using geometric knowledge and the current positions of
   geometries as hints. Specificaly, the strategy is to recursivly

    1. transform as many constraints as possible into simple parameter reductions
        This reduces the number of constraints and parameters given to the numerical solver which
   improves it's stability


        Example: An horizontal line constraint is simply y1 = y2 and is easily handled, but if a
   second line is set to be perpendicular to the first line, then we can remove this rather complex
   constraint by a simple reduction x1 = x2 on the second line.

    2. transform as many constraints as possible into difference constraints
        Since difference constraints are signed, they protect against flips. Using the current
   position of geometries we can assume the correct orientation of the difference and maintain it
   during the solve

        Example: If lineA is of known length (e.g a p2p distance was applied) and lineB is set to be
   of equal length AND is also determined to be horizontal (via point 1), the rather complex equal
   length constraint can be removed and replaced by a difference constraint such that x2+/-lengthA =
   x1 (the sign is determined based on current geometry) which means that lineB will keep it's
   orientation during the solve

    3. transform as many constraints as possible into equal constant constraints
        Equal constraints are easy to solve and stable, even more so when only one of their
   parameter is solvable

*/
struct Substitution
{
    std::vector<Constraint*> constraints;  // Constraints that could not be reduced (may be smaller
                                           // than the initialConstraints received in constructor)
    VEC_D parameters;  // Parameters that mirror unknowns from the GCS class which could not be
                       // reduced/substituted further. The first part of the vector ([0,
                       // unknowns.size[) is fed to the solvers
    VEC_pD unknowns;

    // A parameter can be reduced but not substituted or the other way around
    // reductionMap maps which parameter will replace a given parameter
    // substitutionMap maps which parameter will be asked for when the constraint needs to derive a
    // given parameter both map map from unknowns to parameters
    std::unordered_map<double*, double*> reductionMap;

    std::vector<ConstraintEqual*> constantConstraints;
    std::vector<double> constants;

    std::vector<ConstraintDifference*> differenceConstraints;
    std::vector<double> differences;

    Substitution(const VEC_pD& initialUnknowns, const std::vector<Constraint*>& initialConstraints);
    Substitution(Substitution&& other) noexcept;
    Substitution() = default;

    Substitution& operator=(Substitution&& other) noexcept;

    static Substitution makeTrivial(
        const VEC_pD& initialUnknowns,
        const std::vector<Constraint*>& initialConstraints
    );

    // We have to manage the lifetime of constraints which
    // we have created to substitute given constraints
    ~Substitution();

    // Put unknowns' values into the parameters
    void initParams();
};

}  // namespace GCS

#endif
