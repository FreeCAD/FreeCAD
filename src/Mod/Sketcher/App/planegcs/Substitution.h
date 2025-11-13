
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
   unknowns to the solver in case we mess them up so solver parameters are double that are copied
   from the unknowns, solved, and copied back if successful. The substitution class handles the
   lifetime of the solverParameters so subsystems get pointers to theses values within the vector

    reduced constraint: A constraint that can be expressed as a simple substitution such as x1 =
   x2+a (where a can be 0) and that is handled symbolicaly by the Substitution class. It is reduced
   because it does not appear in any Subsystems and is not solved numericaly

    subsituted parameter: A parameter that can be expressed as x1 = x2 + a. The derivative of a
   function with respect to x1 or x2 is equal (chain rule) so a single parameter (say x2) has to be
   handed down to the solver. In this example, x1 still has to be updated following x1 = x2 + at
   each solver iteration to compute the error matrix. This is advantageous because the numerical
   solver has a complexity of the order O(n^3) so reducing the size of the system impacts
   performance greatly AND this symbolic solves gives directionnality to the geometry

    reduced parameter: A parameter that can be expressed as x1 = x2. This is a special case of a
   substituted parameter that is handled separatly because we can avoid the update step of the
   substituted parameter by simply changing the pointers to the solver parameters in the constraint
   to point to x2 instead of x1

    const parameters: These are parameters whose value can be 100% found using substitution (which
   means that a chain of simple additions link them to the origin) these parameters are not fed to
   the solver and are simply updated at the begining of the solve
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
    UMAP_pD_pD reductionMap;
    UMAP_pD_pD substitutionMap;

    std::vector<SubstitutionUpdater> substitutionUpdaters;
    std::vector<SubstitutionUpdater> constUpdaters;

    Substitution(const VEC_pD& initialUnknowns, const std::vector<Constraint*>& initialConstraints);
    Substitution() = default;

    // Put unknowns' values into the parameters
    void initParams();

    void applyConst() const;
    void applySubst() const;
    void applyReduction() const;
};

}  // namespace GCS

#endif
