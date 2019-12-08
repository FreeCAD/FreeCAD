#include "PreCompiled.h"

#include "SQP.h"
#include "DualMath.h"
#include "PyUtils.h"

#include "Eigen/Dense"

TYPESYSTEM_SOURCE(FCS::SQP, FCS::SolverBackend);

using namespace FCS;


Py::Dict SQP::Prefs::getPyValue() const
{
    Py::Dict ret = SolverBackend::Prefs::getPyValue();
    ret["minStep"] = Py::Float(minStep);
    return ret;
}

void SQP::Prefs::setAttr(std::string attrname, Py::Object value)
{
    if (attrname == "minStep") {
        this->minStep = Py::Float(value);
    }
    else {
        SolverBackend::Prefs::setAttr(attrname, value);
    }
}


// minimizes ( 0.5 * x^T * H * x + g^T * x ) under the condition ( A*x + c = 0 )
// it returns the solution in x, the row-space of A in Y, and the null space of A in Z
int SQP::qp_eq(const Eigen::MatrixXd &H, const Eigen::VectorXd &g, const Eigen::MatrixXd &A, const Eigen::VectorXd &c,
          Eigen::VectorXd &x, Eigen::MatrixXd &Y, Eigen::MatrixXd &Z)
{
    Eigen::FullPivHouseholderQR<Eigen::MatrixXd> qrAT(A.transpose());
    Eigen::MatrixXd Q = qrAT.matrixQ ();

    ssize_t xsize = qrAT.rows();
    ssize_t csize = qrAT.cols();
    ssize_t rank = qrAT.rank();

    if (rank != csize || csize > xsize)
        return -1;

    // A^T = Q*R*P^T = Q1*R1*P^T
    // Q = [Q1,Q2], R=[R1;0]
    // Y = Q1 * inv(R^T) * P^T
    // Z = Q2
    Y = qrAT.matrixQR().topRows(csize)
                       .triangularView<Eigen::Upper>()
                       .transpose()
                       .solve<Eigen::OnTheRight>(Q.leftCols(rank))
        * qrAT.colsPermutation().transpose();
    if (xsize == rank)
        x = - Y * c;
    else {
        Z = Q.rightCols(xsize-rank);

        Eigen::MatrixXd ZTHZ = Z.transpose() * H * Z;
        Eigen::VectorXd rhs = Z.transpose() * (H * Y * c - g);

        Eigen::VectorXd y = ZTHZ.colPivHouseholderQr().solve(rhs);

        x = - Y * c + Z * y;
    }

    return 0;
}

eSolveResult SQP::solve(HSubSystem sys, HValueSet vals)
{
    //not supported, redirectn to solvePair
    return SolverBackend::solve(sys, vals);
}

eSolveResult SQP::solvePair(HSubSystem mainsys, HSubSystem auxsys, HValueSet vals)
{
    iterLog("Begin SQP(two-system) solving");

    if (mainsys->isTouched())
        mainsys->update();
    if (auxsys->isTouched())
        auxsys->update();

    if (!mainsys->params().is(auxsys->params()))
        throw Base::ValueError("solveSQP: Subsystems do not share the set of parameters");
    HParameterSubset params = mainsys->params();

    mainsys->checkValuesCoverage(*vals);

    ssize_t maxIterNumber = _prefs.iterLimit(mainsys);

    int xsize = mainsys->params()->size();
    int csizeA = mainsys->subconstraints().size();
    int csizeB = auxsys->subconstraints().size();

    if(xsize == 0 || (csizeA + csizeB == 0)){
        iterLog("    nothing to do, finished.");
        return eSolveResult::Success;
    }

    Eigen::MatrixXd B = Eigen::MatrixXd::Identity(xsize, xsize);
    Eigen::MatrixXd JA(csizeA, xsize);
    Eigen::MatrixXd Y,Z;

    Eigen::VectorXd resA(csizeA);
    Eigen::VectorXd lambda(csizeA), lambda0(csizeA), lambdadir(csizeA);
    //Eigen::VectorXd x(xsize), x0(xsize), xdir(xsize), xdir1(xsize);
    HValueSet x = ValueSet::makeFrom(params, *vals);
    HValueSet x0 = ValueSet::makeFrom(params, *vals);
    HValueSet xdir = ValueSet::makeZeros(params);
    HValueSet xdir1 = ValueSet::makeZeros(params);

    Eigen::VectorXd grad(xsize);
    Eigen::VectorXd h(xsize);
    Eigen::VectorXd y(xsize);
    Eigen::VectorXd Bh(xsize);

    double err;

    // We assume that there are no common constraints in mainsys and auxsys

    auxsys->calcGrad(vals,grad);
    mainsys->calcJacobi(*vals, params, JA);
    mainsys->calcResidual(*vals, resA, err);

    //double convergence = isFine ? XconvergenceFine : XconvergenceRough;
    double divergingLim = 1e6 * err + 1e12;

    double mu = 0;
    lambda.setZero();
    for (int iter = 1; iter < maxIterNumber; iter++) {
        iterLog("  iteration %i",iter);
        int status = qp_eq(B, grad, JA, resA, xdir->values(), Y, Z);
        if (status){
            throw SolverError("SQP solver failed: qp_eq returned -1, likely because of redundant/conflicting constraints");
        }

        *x0 = x;
        lambda0 = lambda;
        lambda = Y.transpose() * (B * xdir->values() + grad);
        lambdadir = lambda - lambda0;

        // line search
        {
            iterLog("    line search:");
            double eta=0.25;
            double tau=0.5;
            double rho=0.5;
            double alpha=1;
            alpha = std::min(alpha, mainsys->maxStep(*vals, *xdir));

            // Eq. 18.32
            // double mu = lambda.lpNorm<Eigen::Infinity>() + 0.01;
            // Eq. 18.33
            // double mu =  grad.dot(xdir) / ( (1.-rho) * resA.lpNorm<1>());
            // Eq. 18.36
            mu =  std::max(mu,
                           (grad.dot(xdir->values()) +  std::max(0., 0.5*xdir->values().dot(B*xdir->values()))) /
                           ( (1. - rho) * resA.lpNorm<1>() ) );

            // Eq. 18.27
            double f0 = auxsys->error(*vals) + mu * resA.lpNorm<1>();

            // Eq. 18.29
            double deriv = grad.dot(xdir->values()) - mu * resA.lpNorm<1>();

            *x = x0 + alpha * xdir;
            vals->paste(x);
            mainsys->calcResidual(*vals, resA, err);
            double f = auxsys->error(*vals) + mu * resA.lpNorm<1>();
            iterLog("      alpha = %f, err^2 = %f, f = %f", alpha, err, f);

            // line search, Eq. 18.28
            bool first = true;
            while (true) {
                if (first) { // try a second order step
                    iterLog("      attempting 2nd order step");
//                    xdir1 = JA.jacobiSvd(Eigen::ComputeThinU |
//                                         Eigen::ComputeThinV).solve(-resA);
                    *xdir1 = -Y*resA;
                    x->values() += xdir1->values(); // = x0 + alpha * xdir + xdir1
                    vals->paste(x);
                    mainsys->calcResidual(*vals, resA, err);
                    f = auxsys->error(*vals) + mu * resA.lpNorm<1>();
                    iterLog("      err^2 = %f, f = %f", err, f);
                    if (f < f0 + eta * alpha * deriv){
                        iterLog("      second-order step accepted");
                        break;
                    }
                }
                iterLog("      reducing alpha");
                alpha = tau * alpha;
                if (alpha < 1e-8) // let the linesearch fail
                    alpha = 0.;
                *x = x0 + alpha * xdir;
                vals->paste(x);
                mainsys->calcResidual(*vals, resA, err);
                f = auxsys->error(*vals) + mu * resA.lpNorm<1>();
                iterLog("      alpha = %f, err^2 = %f, f = %f", alpha, err, f);
                if (alpha < 1e-8) { // let the linesearch fail
                    iterLog("      alpha is too small, line search failed");
                    break;
                }
                if (f > f0 + eta * alpha * deriv){
                    iterLog("    line search finished");
                    break;
                }
            }
            lambda = lambda0 + alpha * lambdadir;

        }
        h = x->values() - x0->values();

        y = grad - JA.transpose() * lambda;
        {
            auxsys->calcGrad(vals, grad);
            mainsys->calcJacobi(*vals, params, JA);
            mainsys->calcResidual(*vals, resA, err);
        }
        y = grad - JA.transpose() * lambda - y; // Eq. 18.13

        if (iter > 1) {
            double yTh = y.dot(h);
            if (yTh != 0) {
                iterLog("    BFGS update on B. y*h = %f", yTh);
                Bh = B * h;
                //Now calculate the BFGS update on B
                B += 1./yTh * y * y.transpose();
                B -= 1./h.dot(Bh) * (Bh * Bh.transpose());
            }
        }

        err = mainsys->error(*vals);
        iterLog("    sq(err) = %f", err);
        if (h.norm() <= (_prefs.minStep) && err <= sq(_prefs.errorForSolved)){
            iterLog("    main system error is small, and the step was small. Success.");
            break;
        }
        if (err > divergingLim){
            throw SolverError("SQP solver failed: diverged");
        }
        if (err != err) {
            throw SolverError("SQP solver failed: NaN error");
        }
        if (iter == maxIterNumber - 1){
            if(err > sq(_prefs.errorForSolved))
                throw SolverError("SQP solver failed: iteration limit reached, and main system error is still not small");
            Base::Console().Warning("SQP solver: iteration limit reached\n");
            break;
        }
    }

    eSolveResult ret;
    if (mainsys->error(*vals) <= sq(_prefs.errorForSolved))
        ret = eSolveResult::Success;
    else if (h.norm() <= _prefs.minStep)
        ret = eSolveResult::Minimized;
    else
        ret = eSolveResult::Failed; //shouldn't happen, we should have thrown...

    return ret;


}
