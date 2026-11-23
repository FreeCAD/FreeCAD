// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Konstantinos Poulios
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#undef min
#undef max

#include <Eigen/Core>

#include "Constraints.h"


namespace GCS
{

class SubSystem
{
private:
    int psize, csize;
    std::vector<Constraint*> clist;
    VEC_pD plist;    // pointers to the original parameters
    MAP_pD_pD pmap;  // redirection map from the original parameters to pvals
    VEC_D pvals;     // current variables vector (psize)
                     //        JacobianMatrix jacobi;  // jacobi matrix of the residuals
    std::map<Constraint*, VEC_pD> c2p;                // constraint to parameter adjacency list
    std::map<double*, std::vector<Constraint*>> p2c;  // parameter to constraint adjacency list
    void initialize(VEC_pD& params, MAP_pD_pD& reductionmap);  // called by the constructors
public:
    SubSystem(std::vector<Constraint*>& clist_, VEC_pD& params);
    SubSystem(std::vector<Constraint*>& clist_, VEC_pD& params, MAP_pD_pD& reductionmap);
    ~SubSystem();

    int pSize()
    {
        return psize;
    };
    int cSize()
    {
        return csize;
    };

    void redirectParams();
    void revertParams();

    void getParamMap(MAP_pD_pD& pmapOut);
    void getParamList(VEC_pD& plistOut);

    void getParams(VEC_pD& params, Eigen::VectorXd& xOut);
    void getParams(Eigen::VectorXd& xOut);
    void setParams(VEC_pD& params, Eigen::VectorXd& xIn);
    void setParams(Eigen::VectorXd& xIn);

    void getConstraintList(std::vector<Constraint*>& clist_);

    double error();
    void calcResidual(Eigen::VectorXd& r);
    void calcResidual(Eigen::VectorXd& r, double& err);
    void calcJacobi(VEC_pD& params, Eigen::MatrixXd& jacobi);
    void calcJacobi(Eigen::MatrixXd& jacobi);
    void calcGrad(VEC_pD& params, Eigen::VectorXd& grad);
    void calcGrad(Eigen::VectorXd& grad);

    double maxStep(VEC_pD& params, Eigen::VectorXd& xdir);
    double maxStep(Eigen::VectorXd& xdir);

    void applySolution();
    void analyse(Eigen::MatrixXd& J, Eigen::MatrixXd& ker, Eigen::MatrixXd& img);
    void report();

    void printResidual();
};

double lineSearch(SubSystem* subsys, Eigen::VectorXd& xdir);

}  // namespace GCS
