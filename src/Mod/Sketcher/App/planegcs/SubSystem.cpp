// SPDX-License-Identifier: LGPL-2.1-or-later

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

#ifdef _MSC_VER
# pragma warning(disable : 4251)
#endif

#include <iostream>
#include <iterator>

#include "SubSystem.h"


namespace GCS
{

// SubSystem
SubSystem::SubSystem(std::vector<Constraint*>& clist_, VEC_pD& params, const UMAP_pD_pD& unknownToParam_)
    : clist(clist_)
    , plist(params)
    , unknownToParam(unknownToParam_)
{
    csize = static_cast<int>(clist.size());
    psize = static_cast<int>(plist.size());

    // tmpplist will contain the subset of parameters from params that are
    // relevant for the constraints listed in clist
    p2c.clear();
    for (const auto& constr : clist) {
        VEC_pD constr_params_orig = constr->origParams();
        SET_pD constr_params;
        for (const auto& constrParam : constr_params_orig) {
            auto pmapfind = unknownToParam.find(constrParam);
            if (pmapfind != unknownToParam.end()) {
                constr_params.insert(pmapfind->second);
            }
        }
        for (const auto& constrParam : constr_params) {
            p2c[constrParam].push_back(constr);
        }
    }
}

SubSystem::~SubSystem()
{}

void SubSystem::redirectParams()
{
    // redirect constraints to point to pvals
    for (const auto& constr : clist) {
        constr->revertParams();  // this line will normally not be necessary
        constr->redirectParams(unknownToParam);
    }
}

void SubSystem::revertParams()
{
    for (auto constr : clist) {
        constr->revertParams();
    }
}

void SubSystem::getParamList(VEC_pD& plistOut)
{
    plistOut = plist;
}

void SubSystem::getParams(VEC_pD& params, Eigen::VectorXd& xOut)
{
    if (xOut.size() != int(params.size())) {
        xOut.setZero(params.size());
    }

    for (int j = 0; j < int(params.size()); j++) {
        xOut[j] = *params[j];
    }
}

void SubSystem::getParams(Eigen::VectorXd& xOut)
{
    getParams(plist, xOut);
}

void SubSystem::setParams(VEC_pD& params, Eigen::VectorXd& xIn)
{
    assert(xIn.size() == int(params.size()));
    for (int j = 0; j < int(params.size()); j++) {
        *params[j] = xIn[j];
    }
}

void SubSystem::setParams(Eigen::VectorXd& xIn)
{
    setParams(plist, xIn);
}

void SubSystem::getConstraintList(std::vector<Constraint*>& clist_)
{
    clist_ = clist;
}

double SubSystem::error()
{
    double err = 0.;
    for (const auto& constr : clist) {
        double tmp = constr->error();
        err += tmp * tmp;
    }
    err *= 0.5;
    return err;
}

void SubSystem::calcResidual(Eigen::VectorXd& r)
{
    assert(r.size() == csize);

    int i = 0;
    for (const auto& constr : clist) {
        r[i] = constr->error();
        i++;
    }
}

void SubSystem::calcResidual(Eigen::VectorXd& r, double& err)
{
    assert(r.size() == csize);

    int i = 0;
    err = 0.;
    for (const auto& constr : clist) {
        r[i] = constr->error();
        err += r[i] * r[i];
        i++;
    }
    err *= 0.5;
}

void SubSystem::calcJacobi(VEC_pD& params, Eigen::MatrixXd& jacobi)
{
    jacobi.setZero(csize, params.size());
    for (int j = 0; j < int(params.size()); j++) {
        // TODO-theo-vt: check if checking if param is in p2c would be beneficial
        for (int i = 0; i < csize; i++) {
            jacobi(i, j) = clist[i]->grad(params[j]);
        }
    }
}

void SubSystem::calcJacobi(Eigen::MatrixXd& jacobi)
{
    calcJacobi(plist, jacobi);
}

void SubSystem::calcGrad(VEC_pD& params, Eigen::VectorXd& grad)
{
    assert(grad.size() == int(params.size()));

    grad.setZero();
    for (int j = 0; j < int(params.size()); j++) {

        const auto& foundConstrs = p2c.find(params[j]);
        if (foundConstrs == p2c.end()) {
            continue;
        }
        for (auto constr : foundConstrs->second) {
            grad[j] += constr->error() * constr->grad(params[j]);
        }
    }
}

void SubSystem::calcGrad(Eigen::VectorXd& grad)
{
    calcGrad(plist, grad);
}

double SubSystem::maxStep(VEC_pD& params, Eigen::VectorXd& xdir)
{
    assert(xdir.size() == int(params.size()));

    MAP_pD_D dir;
    for (int j = 0; j < int(params.size()); j++) {
        const auto& foundParam = p2c.find(params[j]);
        if (foundParam != p2c.end()) {
            dir[params[j]] = xdir[j];
        }
    }

    double alpha = 1e10;
    for (const auto& constr : clist) {
        alpha = constr->maxStep(dir, alpha);
    }

    return alpha;
}

double SubSystem::maxStep(Eigen::VectorXd& xdir)
{
    return maxStep(plist, xdir);
}

void SubSystem::analyse(Eigen::MatrixXd& /*J*/, Eigen::MatrixXd& /*ker*/, Eigen::MatrixXd& /*img*/)
{}

void SubSystem::report()
{}

void SubSystem::printResidual()
{
    Eigen::VectorXd r(csize);
    int i = 0;
    double err = 0.;
    for (const auto& constr : clist) {
        r[i] = constr->error();
        err += r[i] * r[i];
        i++;
    }
    err *= 0.5;
    std::cout << "Residual r = " << r.transpose() << std::endl;
    std::cout << "Residual err = " << err << std::endl;
}


}  // namespace GCS
