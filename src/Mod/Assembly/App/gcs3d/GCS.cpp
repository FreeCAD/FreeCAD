/***************************************************************************
 *   Copyright (c) Konstantinos Poulios      (logari81@gmail.com) 2011     *
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
#include <iostream>
#include <algorithm>
#include <cfloat>

#include "GCS.h"
#include "qp_eq.h"
#include <Eigen/QR>

#include <float.h>
#include <stdio.h>
#include <time.h>

namespace GCS
{


///////////////////////////////////////
// Solver
///////////////////////////////////////

// System
System::System()
        : clist(0),
        c2p(), p2c(),
        subsys0(0),
        subsys1(0),
        subsys2(0),
        reference(),
        init(false)
{
}

System::System(std::vector<Constraint *> clist_)
        : c2p(), p2c(),
        subsys0(0),
        subsys1(0),
        subsys2(0),
        reference(),
        init(false)
{
    // create own (shallow) copy of constraints
    for (std::vector<Constraint *>::iterator constr=clist_.begin();
            constr != clist_.end(); ++constr) {
        Constraint *newconstr;
        switch ((*constr)->getTypeId()) {
 
	case GCS::ASParallel: {
	    ConstraintParralelFaceAS *oldconstr = static_cast<ConstraintParralelFaceAS *>(*constr);
            newconstr = new ConstraintParralelFaceAS(*oldconstr);
	    break;
	}
	case GCS::ASDistance: {
	    ConstraintFaceDistanceAS *oldconstr = static_cast<ConstraintFaceDistanceAS *>(*constr);
            newconstr = new ConstraintFaceDistanceAS(*oldconstr);
	    break;
	}
        case None:
            break;
        }
        if (newconstr)
            addConstraint(newconstr);
    }
}

System::~System()
{
    clear();
}

void System::clear()
{
    clearReference();
    clearSubSystems();
    free(clist);
    c2p.clear();
    p2c.clear();
}

void System::clearByTag(int tagId)
{
    std::vector<Constraint *> constrvec;
    for (std::vector<Constraint *>::const_iterator
            constr=clist.begin(); constr != clist.end(); ++constr) {
        if ((*constr)->getTag() == tagId)
            constrvec.push_back(*constr);
    }
    for (std::vector<Constraint *>::const_iterator
            constr=constrvec.begin(); constr != constrvec.end(); ++constr) {
        removeConstraint(*constr);
    }
}

int System::addConstraint(Constraint *constr)
{
    clearReference();

    clist.push_back(constr);
    VEC_pD constr_params = constr->params();
    for (VEC_pD::const_iterator param=constr_params.begin();
            param != constr_params.end(); ++param) {
//        jacobi.set(constr, *param, 0.);
        c2p[constr].push_back(*param);
        p2c[*param].push_back(constr);
    }
    return clist.size()-1;
}

void System::removeConstraint(Constraint *constr)
{
    clearReference();
    clearSubSystems();

    std::vector<Constraint *>::iterator it;
    it = std::find(clist.begin(), clist.end(), constr);
    clist.erase(it);

    VEC_pD constr_params = c2p[constr];
    for (VEC_pD::const_iterator param=constr_params.begin();
            param != constr_params.end(); ++param) {
        std::vector<Constraint *> &constraints = p2c[*param];
        it = std::find(constraints.begin(), constraints.end(), constr);
        constraints.erase(it);
    }
    c2p.erase(constr);

    std::vector<Constraint *> constrvec;
    constrvec.push_back(constr);
    free(constrvec);
}



void System::initSolution(VEC_pD &params)
{
    // - Stores the current parameters in the vector "reference"
    // - Identifies the equality constraints tagged with ids >= 0
    //   and prepares a corresponding system reduction
    // - Organizes the rest of constraints into two subsystems for
    //   tag ids >=0 and < 0 respectively and applies the
    //   system reduction specified in the previous step

    clearReference();
    for (VEC_pD::const_iterator param=params.begin();
            param != params.end(); ++param)
        reference[*param] = **param;

    // identification of equality constraints and parameter reduction
    std::set<Constraint *> eliminated;  // constraints that will be eliminated through reduction
    reductionmap.clear();
    {
        VEC_pD reduced_params=params;
        MAP_pD_I params_index;
        for (int i=0; i < int(params.size()); ++i)
            params_index[params[i]] = i;

    /*    for (std::vector<Constraint *>::const_iterator constr=clist.begin();
                constr != clist.end(); ++constr) {
            if ((*constr)->getTag() >= 0 && (*constr)->getTypeId() == Equal) {
                MAP_pD_I::const_iterator it1,it2;
                it1 = params_index.find((*constr)->params()[0]);
                it2 = params_index.find((*constr)->params()[1]);
                if (it1 != params_index.end() && it2 != params_index.end()) {
                    eliminated.insert(*constr);
                    double *p_kept = reduced_params[it1->second];
                    double *p_replaced = reduced_params[it2->second];
                    for (int i=0; i < int(params.size()); ++i)
                        if (reduced_params[i] == p_replaced)
                            reduced_params[i] = p_kept;
                }
            }
        }*/
        for (int i=0; i < int(params.size()); ++i)
            if (params[i] != reduced_params[i])
                reductionmap[params[i]] = reduced_params[i];
    }

    int i=0;
    std::vector<Constraint *> clist0, clist1, clist2;
    for (std::vector<Constraint *>::const_iterator constr=clist.begin();
            constr != clist.end(); ++constr, i++) {
        if (eliminated.count(*constr) == 0) {
            if ((*constr)->getTag() >= 0)
                clist0.push_back(*constr);
            else if ((*constr)->getTag() == -1) // move constraints
                clist1.push_back(*constr);
            else // distance from reference constraints
                clist2.push_back(*constr);
        }
    }

    clearSubSystems();
    if (clist0.size() > 0)
        subsys0 = new SubSystem(clist0, params, reductionmap);
    if (clist1.size() > 0)
        subsys1 = new SubSystem(clist1, params, reductionmap);
    if (clist2.size() > 0)
        subsys2 = new SubSystem(clist2, params, reductionmap);
    init = true;
}

void System::clearReference()
{
    init = false;
    reference.clear();
}

void System::resetToReference()
{
    for (MAP_pD_D::const_iterator it=reference.begin();
            it != reference.end(); ++it)
        *(it->first) = it->second;
}

int System::solve(VEC_pD &params, bool isFine, Algorithm alg)
{
    initSolution(params);
    return solve(isFine, alg);
}

int System::solve(bool isFine, Algorithm alg)
{
    if (subsys0) {
        resetToReference();
        if (subsys2) {
            int ret = solve(subsys0, subsys2, isFine);
            if (subsys1) // give subsys1 higher priority than subsys2
                // in this case subsys2 acts like a preconditioner
                return solve(subsys0, subsys1, isFine);
            else
                return ret;
        }
        else if (subsys1)
            return solve(subsys0, subsys1, isFine);
        else
            return solve(subsys0, isFine, alg);
    }
    else if (subsys1) {
        resetToReference();
        if (subsys2)
            return solve(subsys1, subsys2, isFine);
        else
            return solve(subsys1, isFine, alg);
    }
    else
        // return success in order to permit coincidence constraints to be applied
        return Success;
}

int System::solve(SubSystem *subsys, bool isFine, Algorithm alg)
{
  
    std::cout << std::endl << "Initial Error: " << subsys->error() << std::endl;
    clock_t begin = clock();
    int ret;
    if (alg == BFGS)
        ret= solve_BFGS(subsys, isFine);
    else if (alg == LevenbergMarquardt)
        ret= solve_LM(subsys);
    else if (alg == DogLeg)
        ret= solve_DL(subsys);
    else if (alg == HOPS)
        ret= solve_EX(subsys);
    
    clock_t end = clock();
    std::cout << "Time elapsed: " << double( ((end-begin)*1000)/CLOCKS_PER_SEC ) << " ms"<< std::endl;

    return ret;
}

int System::solve_EX(SubSystem* subsys) {


    return 0;

}


int System::solve_BFGS(SubSystem *subsys, bool isFine)
{
    int xsize = subsys->pSize();
    if (xsize == 0)
        return Success;

    subsys->redirectParams();

    Eigen::MatrixXd D = Eigen::MatrixXd::Identity(xsize, xsize);
    Eigen::VectorXd x(xsize);
    Eigen::VectorXd xdir(xsize);
    Eigen::VectorXd grad(xsize);
    Eigen::VectorXd h(xsize);
    Eigen::VectorXd y(xsize);
    Eigen::VectorXd Dy(xsize);

    // Initial unknowns vector and initial gradient vector
    subsys->getParams(x);
    subsys->calcGrad(grad);

    // Initial search direction oposed to gradient (steepest-descent)
    xdir = grad;
    lineSearch(subsys, xdir);
    double err = subsys->error();

    h = x;
    subsys->getParams(x);
    h = x - h; // = x xold

    double convergence = isFine ? XconvergenceFine : XconvergenceRough;
    int maxIterNumber = MaxIterations * xsize;
    double diverging_lim = 1e6*err + 1e12;

    for (int iter=1; iter < maxIterNumber; iter++) {

        if (h.norm() <= convergence || err <= smallF)
            break;
        if (err > diverging_lim || err != err) // check for diverging and NaN
            break;

        y = grad;
        subsys->calcGrad(grad);
        y = grad - y; // = grad gradold

        double hty = h.dot(y);
        //make sure that hty is never 0
        if (hty == 0)
            hty = .0000000001;

        Dy = D * y;

        double ytDy = y.dot(Dy);

        //Now calculate the BFGS update on D
        D += (1.+ytDy/hty)/hty * h * h.transpose();
        D -= 1./hty * (h * Dy.transpose() + Dy * h.transpose());

        xdir = D * grad;
        lineSearch(subsys, xdir);
        err = subsys->error();

        h = x;
        subsys->getParams(x);
        h = x - h; // = x xold
    }

    subsys->revertParams();
    
    std::cout  << "Error after BFGS solving: " << err << std::endl;

    if (err <= smallF)
        return Success;
    if (h.norm() <= convergence)
        return Converged;
    return Failed;
}

int System::solve_LM(SubSystem* subsys)
{
    int xsize = subsys->pSize();
    int csize = subsys->cSize();

    if (xsize == 0)
        return Success;

    Eigen::VectorXd e(csize), e_new(csize); // vector of all function errors (every constraint is one function)
    Eigen::MatrixXd J(csize, xsize);        // Jacobi of the subsystem
    Eigen::MatrixXd A(xsize, xsize);
    Eigen::VectorXd x(xsize), h(xsize), x_new(xsize), g(xsize), diag_A(xsize);

    subsys->redirectParams();

    subsys->getParams(x);
    subsys->calcResidual(e);
    e*=-1;

    int maxIterNumber = MaxIterations * xsize;
    double diverging_lim = 1e6*e.squaredNorm() + 1e12;

    double eps=1e-10, eps1=1e-80;
    double tau=1e-3;
    double nu=2, mu=0;
    int iter=0, stop=0;
    for (iter=0; iter < maxIterNumber && !stop; ++iter) {

        // check error
        double err=e.squaredNorm();
        if (err <= eps) { // error is small, Success
            stop = 1;
            break;
        }
        else if (err > diverging_lim || err != err) { // check for diverging and NaN
            stop = 6;
            break;
        }

        // J^T J, J^T e
        subsys->calcJacobi(J);;

        A = J.transpose()*J;
        g = J.transpose()*e;

        // Compute ||J^T e||_inf
        double g_inf = g.lpNorm<Eigen::Infinity>();
        diag_A = A.diagonal(); // save diagonal entries so that augmentation can be later canceled

        // check for convergence
        if (g_inf <= eps1) {
            stop = 2;
            break;
        }

        // compute initial damping factor
        if (iter == 0)
            mu = tau * A.diagonal().lpNorm<Eigen::Infinity>();

        // determine increment using adaptive damping
        while (1) {
            // augment normal equations A = A+uI
            for (int i=0; i < xsize; ++i)
                A(i,i) += mu;

            //solve augmented functions A*h=-g
            h = A.fullPivLu().solve(g);
            double rel_error = (A*h - g).norm() / g.norm();

            // check if solving works
            if (rel_error < 1e-5) {
/*
                double scale = subsys->maxStep() / h.lpNorm<Eigen::Infinity>();
                if ( scale < 1.)
                    h *= scale;
*/
                // compute par's new estimate and ||d_par||^2
                x_new = x + h;
                double h_norm = h.squaredNorm();

                if (h_norm <= eps1*eps1*x.norm()) { // relative change in p is small, stop
                    stop = 3;
                    break;
                }
                else if (h_norm >= (x.norm()+eps1)/(DBL_EPSILON*DBL_EPSILON)) { // almost singular
                    stop = 4;
                    break;
                }

                subsys->setParams(x_new);
                subsys->calcResidual(e_new);
                e_new *= -1;

                double dF = e.squaredNorm() - e_new.squaredNorm();
                double dL = h.dot(mu*h+g);

                if (dF>0. && dL>0.) { // reduction in error, increment is accepted
                    double tmp=2*dF/dL-1.;
                    mu *= std::max(1./3., 1.-tmp*tmp*tmp);
                    nu=2;

                    // update par's estimate
                    x = x_new;
                    e = e_new;
                    break;
                }
            }

            // if this point is reached, either the linear system could not be solved or
            // the error did not reduce; in any case, the increment must be rejected

            mu*=nu;
            nu*=2.0;
            for (int i=0; i < xsize; ++i) // restore diagonal J^T J entries
                A(i,i) = diag_A(i);
        }
    }

    if (iter >= maxIterNumber)
        stop = 5;

    std::cout << "Error after LM solving: " << subsys->error() << std::endl;

    subsys->revertParams();
   
    return (stop == 1) ? Success : Failed;
}


int System::solve_DL(SubSystem* subsys)
{
    double tolg=1e-80, tolx=1e-80, tolf=1e-10;

    int xsize = subsys->pSize();
    int csize = subsys->cSize();

    Eigen::VectorXd x(xsize), x_new(xsize);
    Eigen::VectorXd fx(csize), fx_new(csize);
    Eigen::MatrixXd Jx(csize, xsize), Jx_new(csize, xsize);
    Eigen::VectorXd g(xsize), h_sd(xsize), h_gn(xsize), h_dl(xsize);

    subsys->redirectParams();

    double err;
    subsys->getParams(x);
    subsys->calcResidual(fx, err);
    subsys->calcJacobi(Jx);

    g = Jx.transpose()*(-fx);

    // get the infinity norm fx_inf and g_inf
    double g_inf = g.lpNorm<Eigen::Infinity>();
    double fx_inf = fx.lpNorm<Eigen::Infinity>();

    int maxIterNumber = MaxIterations * xsize;
    double diverging_lim = 1e6*err + 1e12;

    double delta=0.1;
    double alpha=0.;
    double nu=2.;
    int iter=0, stop=0, reduce=0;
    while (!stop) {

        // check if finished
        if (fx_inf <= tolf) // Success
            stop = 1;
        else if (g_inf <= tolg)
            stop = 2;
        else if (delta <= tolx*(tolx + x.norm()))
            stop = 3;
        else if (iter >= maxIterNumber)
            stop = 4;
        else if (err > diverging_lim || err != err) { // check for diverging and NaN
            stop = 6;
        }
        else {
            // get the steepest descent direction
            alpha = g.squaredNorm()/(Jx*g).squaredNorm();
            h_sd  = alpha*g;

            // get the gauss-newton step
            h_gn = Jx.fullPivLu().solve(-fx);
            double rel_error = (Jx*h_gn + fx).norm() / fx.norm();
            if (rel_error > 1e15)
                break;

            // compute the dogleg step
            if (h_gn.norm() < delta) {
                h_dl = h_gn;
                if  (h_dl.norm() <= tolx*(tolx + x.norm())) {
                    stop = 5;
                    break;
                }
            }
            else if (alpha*g.norm() >= delta) {
                h_dl = (delta/(alpha*g.norm()))*h_sd;
            }
            else {
                //compute beta
                double beta = 0;
                Eigen::VectorXd b = h_gn - h_sd;
                double bb = (b.transpose()*b).norm();
                double gb = (h_sd.transpose()*b).norm();
                double c = (delta + h_sd.norm())*(delta - h_sd.norm());

                if (gb > 0)
                    beta = c / (gb + sqrt(gb * gb + c * bb));
                else
                    beta = (sqrt(gb * gb + c * bb) - gb)/bb;

                // and update h_dl and dL with beta
                h_dl = h_sd + beta*b;
            }
        }

        // see if we are already finished
        if (stop)
            break;
/*
        double scale = subsys->maxStep() / h_dl.lpNorm<Eigen::Infinity>();
        if ( scale < 1.)
            h_dl *= scale;
*/
        // get the new values
        double err_new;
        x_new = x + h_dl;
        subsys->setParams(x_new);
        subsys->calcResidual(fx_new, err_new);
        subsys->calcJacobi(Jx_new);

        // calculate the linear model and the update ratio
        double dL = err - 0.5*(fx + Jx*h_dl).squaredNorm();
        double dF = err - err_new;
        double rho = dL/dF;

        if (dF > 0 && dL > 0) {
            x  = x_new;
            Jx = Jx_new;
            fx = fx_new;
            err = err_new;

            g = Jx.transpose()*(-fx);

            // get infinity norms
            g_inf = g.lpNorm<Eigen::Infinity>();
            fx_inf = fx.lpNorm<Eigen::Infinity>();
        }
        else
            rho = -1;

        // update delta
        if (fabs(rho-1.) < 0.2 && h_dl.norm() > delta/3. && reduce <= 0) {
            delta = 3*delta;
            nu = 2;
            reduce = 0;
        }
        else if (rho < 0.25) {
            delta = delta/nu;
            nu = 2*nu;
            reduce = 2;
        }
        else
            reduce--;

        // count this iteration and start again
        iter++;
    }
    
    std::cout << "Error after DogLeg solving: " << subsys->error() << std::endl;


    subsys->revertParams();

    return (stop == 1) ? Success : Failed;
}

// The following solver variant solves a system compound of two subsystems
// treating the first of them as of higher priority than the second
int System::solve(SubSystem *subsysA, SubSystem *subsysB, bool isFine)
{
        int xsizeA = subsysA->pSize();
        int xsizeB = subsysB->pSize();
        int csizeA = subsysA->cSize();

        VEC_pD plist(xsizeA+xsizeB);
        {
            VEC_pD plistA, plistB;
            subsysA->getParamList(plistA);
            subsysB->getParamList(plistB);

            std::sort(plistA.begin(),plistA.end());
            std::sort(plistB.begin(),plistB.end());

            VEC_pD::const_iterator it;
            it = std::set_union(plistA.begin(),plistA.end(),
                                plistB.begin(),plistB.end(),plist.begin());
            plist.resize(it-plist.begin());
        }
        int xsize = plist.size();

        Eigen::MatrixXd B = Eigen::MatrixXd::Identity(xsize, xsize);
        Eigen::MatrixXd JA(csizeA, xsize);
        Eigen::MatrixXd Y,Z;

        Eigen::VectorXd resA(csizeA);
        Eigen::VectorXd lambda(csizeA), lambda0(csizeA), lambdadir(csizeA);
        Eigen::VectorXd x(xsize), x0(xsize), xdir(xsize), xdir1(xsize);
        Eigen::VectorXd grad(xsize);
        Eigen::VectorXd h(xsize);
        Eigen::VectorXd y(xsize);
        Eigen::VectorXd Bh(xsize);

        // We assume that there are no common constraints in subsysA and subsysB
        subsysA->redirectParams();
        subsysB->redirectParams();

        subsysB->getParams(plist,x);
        subsysA->getParams(plist,x);
        subsysB->setParams(plist,x);  // just to ensure that A and B are synchronized

        subsysB->calcGrad(plist,grad);
        subsysA->calcJacobi(plist,JA);
        subsysA->calcResidual(resA);

        double convergence = isFine ? XconvergenceFine : XconvergenceRough;
        int maxIterNumber = MaxIterations * xsize;
        double diverging_lim = 1e6*subsysA->error() + 1e12;

        double mu = 0;
        lambda.setZero();
        for (int iter=1; iter < maxIterNumber; iter++) {
            int status = qp_eq(B, grad, JA, resA, xdir, Y, Z);
            if (status)
                break;

            x0 = x;
            lambda0 = lambda;
            lambda = Y.transpose() * (B * xdir + grad);
            lambdadir = lambda - lambda0;

            // line search
            {
                double eta=0.25;
                double tau=0.5;
                double rho=0.5;
                double alpha=1;
                alpha = std::min(alpha, subsysA->maxStep(plist,xdir));

                // Eq. 18.32
                // double mu = lambda.lpNorm<Eigen::Infinity>() + 0.01;
                // Eq. 18.33
                // double mu =  grad.dot(xdir) / ( (1.-rho) * resA.lpNorm<1>());
                // Eq. 18.36
                mu =  std::max(mu,
                               (grad.dot(xdir) +  std::max(0., 0.5*xdir.dot(B*xdir))) /
                               ( (1. - rho) * resA.lpNorm<1>() ) );

                // Eq. 18.27
                double f0 = subsysB->error() + mu * resA.lpNorm<1>();

                // Eq. 18.29
                double deriv = grad.dot(xdir) - mu * resA.lpNorm<1>();

                x = x0 + alpha * xdir;
                subsysA->setParams(plist,x);
                subsysB->setParams(plist,x);
                subsysA->calcResidual(resA);
                double f = subsysB->error() + mu * resA.lpNorm<1>();

                // line search, Eq. 18.28
                bool first = true;
                while (f > f0 + eta * alpha * deriv) {
                    if (first) { // try a second order step
    //                    xdir1 = JA.jacobiSvd(Eigen::ComputeThinU |
    //                                         Eigen::ComputeThinV).solve(-resA);
                        xdir1 = -Y*resA;
                        x += xdir1; // = x0 + alpha * xdir + xdir1
                        subsysA->setParams(plist,x);
                        subsysB->setParams(plist,x);
                        subsysA->calcResidual(resA);
                        f = subsysB->error() + mu * resA.lpNorm<1>();
                        if (f < f0 + eta * alpha * deriv)
                            break;
                    }
                    alpha = tau * alpha;
                    x = x0 + alpha * xdir;
                    subsysA->setParams(plist,x);
                    subsysB->setParams(plist,x);
                    subsysA->calcResidual(resA);
                    f = subsysB->error() + mu * resA.lpNorm<1>();
                }
                lambda = lambda0 + alpha * lambdadir;

            }
            h = x - x0;

            y = grad - JA.transpose() * lambda;
            {
                subsysB->calcGrad(plist,grad);
                subsysA->calcJacobi(plist,JA);
                subsysA->calcResidual(resA);
            }
            y = grad - JA.transpose() * lambda - y; // Eq. 18.13

            if (iter > 1) {
                double yTh = y.dot(h);
                if (yTh != 0) {
                    Bh = B * h;
                    //Now calculate the BFGS update on B
                    B += 1./yTh * y * y.transpose();
                    B -= 1./h.dot(Bh) * (Bh * Bh.transpose());
                }
            }

            double err = subsysA->error();
            if (h.norm() <= convergence && err <= smallF)
                break;
            if (err > diverging_lim || err != err) // check for diverging and NaN
                break;
        }

        int ret;
        if (subsysA->error() <= smallF)
            ret = Success;
        else if (h.norm() <= convergence)
            ret = Converged;
        else
            ret = Failed;

        subsysA->revertParams();
        subsysB->revertParams();
        return ret;
    
}

void System::getSubSystems(std::vector<SubSystem *> &subsysvec)
{
    subsysvec.clear();
    if (subsys0)
        subsysvec.push_back(subsys0);
    if (subsys1)
        subsysvec.push_back(subsys1);
    if (subsys2)
        subsysvec.push_back(subsys2);
}

void System::applySolution()
{
    if (subsys2)
        subsys2->applySolution();
    if (subsys1)
        subsys1->applySolution();
    if (subsys0)
        subsys0->applySolution();

    for (MAP_pD_pD::const_iterator it=reductionmap.begin();
            it != reductionmap.end(); ++it)
        *(it->first) = *(it->second);
}

int System::diagnose(VEC_pD &params, VEC_I &conflicting)
{
    // Analyses the constrainess grad of the system and provides feedback
    // The vector "conflicting" will hold a group of conflicting constraints
    conflicting.clear();
    std::vector<VEC_I> conflictingIndex;
    VEC_I tags;
    Eigen::MatrixXd J(clist.size(), params.size());
    int count=0;
    for (std::vector<Constraint *>::iterator constr=clist.begin();
            constr != clist.end(); ++constr) {
        (*constr)->revertParams();
        if ((*constr)->getTag() >= 0) {
            count++;
            tags.push_back((*constr)->getTag());
            for (int j=0; j < int(params.size()); j++)
                J(count-1,j) = (*constr)->grad(params[j]);
        }
    }

    if (J.rows() > 0) {
        Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qrJT(J.topRows(count).transpose());
        Eigen::MatrixXd Q = qrJT.matrixQ ();
        int params_num = qrJT.rows();
        int constr_num = qrJT.cols();
        int rank = qrJT.rank();

        Eigen::MatrixXd R;
        if (constr_num >= params_num)
            R = qrJT.matrixQR().triangularView<Eigen::Upper>();
        else
            R = qrJT.matrixQR().topRows(constr_num)
                .triangularView<Eigen::Upper>();

        if (constr_num > rank) { // conflicting constraints
            for (int i=1; i < rank; i++) {
                // eliminate non zeros above pivot
                assert(R(i,i) != 0);
                for (int row=0; row < i; row++) {
                    if (R(row,i) != 0) {
                        double coef=R(row,i)/R(i,i);
                        R.block(row,i+1,1,constr_num-i-1) -= coef * R.block(i,i+1,1,constr_num-i-1);
                        R(row,i) = 0;
                    }
                }
            }
            conflictingIndex.resize(constr_num-rank);
            for (int j=rank; j < constr_num; j++) {
                for (int row=0; row < rank; row++) {
                    if (R(row,j) != 0) {
                        int orig_col = qrJT.colsPermutation().indices()[row];
                        conflictingIndex[j-rank].push_back(orig_col);
                    }
                }
                int orig_col = qrJT.colsPermutation().indices()[j];
                conflictingIndex[j-rank].push_back(orig_col);
            }

            SET_I tags_set;
            for (int i=0; i < conflictingIndex.size(); i++) {
                for (int j=0; j < conflictingIndex[i].size(); j++) {
                    tags_set.insert(tags[conflictingIndex[i][j]]);
                }
            }
            tags_set.erase(0); // exclude constraints tagged with zero
            conflicting.resize(tags_set.size());
            std::copy(tags_set.begin(), tags_set.end(), conflicting.begin());

            if (params_num == rank) // over-constrained
                return params_num - constr_num;
        }

        return params_num - rank;
    }
    return params.size();
}

void System::clearSubSystems()
{
    init = false;
    std::vector<SubSystem *> subsystems;
    getSubSystems(subsystems);
    free(subsystems);
    subsys0 = NULL;
    subsys1 = NULL;
    subsys2 = NULL;
}


//calculates the wolfe condition test functions p and g
Eigen::Vector2d PG(SubSystem* sys, Eigen::VectorXd xdir, double sigma) {

    int xsize = sys->pSize();
    Eigen::Vector2d PG;
    Eigen::VectorXd x(xsize), x_new(xsize);
    Eigen::VectorXd gr(xsize), grn(xsize);
    double fx, fxn;

    sys->getParams(x);
    fx = sys->error();
    sys->calcGrad(gr);
    x_new = x-sigma*xdir;
    sys->setParams(x_new);
    fxn = sys->error();
    sys->calcGrad(grn);
    sys->setParams(x);

    if (sigma == 0)
        PG(1) = 1;
    else
        PG(1) = (fx-fxn) / (sigma * gr.transpose()*xdir).norm();

    PG(0) = (grn.transpose()*xdir).norm() / (gr.transpose()*xdir).norm();

    return PG;
}


double lineSearch(SubSystem *subsys, Eigen::VectorXd &xdir)
{
    double alpha, beta;
    int xsize = subsys->pSize();



    //get inital alpha and beta interval
    //**********************************
    double c3 = 0.1, c4 = 5;
    double delta = 0.01, kappa = 0.9, mu = 0.8, sigma, gamma;
    double fx, fxmd;
    bool run = true;

    Eigen::VectorXd x(xsize), x_new(xsize);
    Eigen::VectorXd grad(xsize);


    subsys->getParams(x);
    subsys->calcGrad(grad);
    fx = subsys->error();
    x_new = x-xdir;
    subsys->setParams(x_new);
    fxmd= subsys->error();
    subsys->setParams(x);

    //calculate an guess for sigma
    double comp1 = (grad.transpose()*xdir).norm() / std::pow(xdir.norm(),2);
    double comp2 = (grad.transpose()*xdir).norm() /(2*(fxmd-fx+(grad.transpose()*xdir).norm()));
    sigma = std::min(c4*comp1 , std::max(c3*comp1, comp2));

    //calculate test functions with guess
    Eigen::Vector2d pg = PG(subsys, xdir, sigma);

    //see if we are alread fisished
    if ( pg(1)>=delta && pg(0) <= kappa)
        run = false;
    else if ( pg(1)>=delta && pg(0)>kappa) {
        alpha = sigma;
        //beta is sigma/mu^j with j minimal so that g(beta)<delta (j element natural numbers)
        for (int i=1; i<=1000; i++) {
            beta = sigma/(std::pow(mu, i));
            pg = PG(subsys, xdir, beta);
            if (pg(1)<delta) break;
        }
    }
    else {
        beta = sigma;
        //alpha is sigma*mu^j with j minimal so that g(alpha)>=delta and p(alpha)>=kappa (j element natural numbers)
        for (int i=1; i<=1000; i++) {
            alpha = sigma*(std::pow(mu, i));
            pg = PG(subsys, xdir, alpha);
            if (pg(0)>=kappa && pg(1) >= delta) break;
        }
    }

    //start sigma search in interval alpha beta
    //*****************************************
    while (run) {

        gamma = (alpha + beta)/2.;
        pg = PG(subsys, xdir, gamma);

        if (pg(0) > kappa && pg(1) >= delta) alpha = gamma;
        else if (pg(0) <= kappa && pg(1) >= delta) {
            sigma = gamma;
            run = false;
        }
        else beta = gamma;

        if (alpha == beta) break;
    }

    x_new = x - sigma* xdir;
    subsys->setParams(x_new);

    std::cout << "sigma: "<<sigma << std::endl;

    return sigma;
}
/*
double lineSearch(SubSystem *subsys, Eigen::VectorXd &xdir)
{
    double f1,f2,f3,alpha1,alpha2,alpha3,alphaStar;

    double alphaMax = subsys->maxStep(xdir);

    Eigen::VectorXd x0, x;

    //Save initial values
    subsys->getParams(x0);

    //Start at the initial position alpha1 = 0
    alpha1 = 0.;
    f1 = subsys->error();

    //Take a step of alpha2 = 1
    alpha2 = 1.;
    x = x0 + alpha2 * xdir;
    subsys->setParams(x);
    f2 = subsys->error();

    //Take a step of alpha3 = 2*alpha2
    alpha3 = alpha2*2;
    x = x0 + alpha3 * xdir;
    subsys->setParams(x);
    f3 = subsys->error();

    //Now reduce or lengthen alpha2 and alpha3 until the minimum is
    //Bracketed by the triplet f1>f2<f3
    while (f2 > f1 || f2 > f3) {
        if (f2 > f1) {
            //If f2 is greater than f1 then we shorten alpha2 and alpha3 closer to f1
            //Effectively both are shortened by a factor of two.
            alpha3 = alpha2;
            f3 = f2;
            alpha2 = alpha2 / 2;
            x = x0 + alpha2 * xdir;
            subsys->setParams(x);
            f2 = subsys->error();
        }
        else if (f2 > f3) {
            if (alpha3 >= alphaMax)
                break;
            //If f2 is greater than f3 then we increase alpha2 and alpha3 away from f1
            //Effectively both are lengthened by a factor of two.
            alpha2 = alpha3;
            f2 = f3;
            alpha3 = alpha3 * 2;
            x = x0 + alpha3 * xdir;
            subsys->setParams(x);
            f3 = subsys->error();
        }
    }
    //Get the alpha for the minimum f of the quadratic approximation
    alphaStar = alpha2 + ((alpha2-alpha1)*(f1-f3))/(3*(f1-2*f2+f3));

    //Guarantee that the new alphaStar is within the bracket
    if (alphaStar >= alpha3 || alphaStar <= alpha1)
        alphaStar = alpha2;

    if (alphaStar > alphaMax)
        alphaStar = alphaMax;

    if (alphaStar != alphaStar)
        alphaStar = 0.;

    //Take a final step to alphaStar
    x = x0 + alphaStar * xdir;
    subsys->setParams(x);

    return alphaStar;
}*/


void free(VEC_pD &doublevec)
{
    for (VEC_pD::iterator it = doublevec.begin();
            it != doublevec.end(); ++it)
        if (*it) delete *it;
    doublevec.clear();
}

void free(std::vector<Constraint *> &constrvec)
{
    for (std::vector<Constraint *>::iterator constr=constrvec.begin();
            constr != constrvec.end(); ++constr) {
        if (*constr) {
            switch ((*constr)->getTypeId()) {
           
            case None:
            default:
                delete *constr;
            }
        }
    }
    constrvec.clear();
}

void free(std::vector<SubSystem *> &subsysvec)
{
    for (std::vector<SubSystem *>::iterator it=subsysvec.begin();
            it != subsysvec.end(); ++it)
        if (*it) delete *it;
}

} //namespace GCS
