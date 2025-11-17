/***************************************************************************
 *   Copyright (c) 2011 Konstantinos Poulios <logari81@gmail.com>          *
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

#ifndef PLANEGCS_UTIL_H
#define PLANEGCS_UTIL_H

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace GCS
{

using VEC_pD = std::vector<double*>;
using VEC_D = std::vector<double>;
using VEC_I = std::vector<int>;
using MAP_pD_pD = std::map<double*, double*>;
using MAP_pD_D = std::map<double*, double>;
using MAP_pD_I = std::map<double*, int>;
using UMAP_pD_pD = std::unordered_map<double*, double*>;
using UMAP_pD_I = std::unordered_map<double*, int>;
using SET_pD = std::set<double*>;
using SET_I = std::set<int>;
using USET_pD = std::unordered_set<double*>;


// The chain rule tells us that if a variable is defined such that
// A = B + const offset, df/dA == df/B, so only one of those variables
// would need to be solved for by the numerical solver
// and we want to reduce the number of solved parameters because numerical
// solvers have a computation complexity on the order of O(n^3) and
// we can use geometric understanding to solve the rest in a way
// that is more "intuitive" for users

// Describes a parameter of the constraint and it's associated
// solver parameter. The number of solver parameter is smaller or
// equal to the number of constraints parameters.
// The solver will ask for a the derivative of the constraint with
// respect to a solver parameter and the constraint will compare this
// parameter to the 'deri' member to choose the computation branch but
// still use 'param' to execute the actual computation.
struct DeriParam
{
    double* param {nullptr};
    double* deri {nullptr};

    DeriParam() = default;
    DeriParam(double* param_)
        : param(param_)
        , deri(param_)
    {}
    DeriParam(double* param_, double* deri_)
        : param(param_)
        , deri(deri_)
    {}
    operator double*() const
    {
        return param;
    }
    double& operator*() const
    {
        return *param;
    }
};

using VEC_Deri = std::vector<DeriParam>;

}  // namespace GCS

#endif  // PLANEGCS_UTIL_H
