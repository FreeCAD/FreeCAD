#include "PreCompiled.h"

#include "BFGS.h"
#include "DualMath.h"

#include "Eigen/Dense"

TYPESYSTEM_SOURCE(FCS::BFGS, FCS::SolverBackend);

using namespace FCS;


Py::Dict BFGS::Prefs::getPyValue() const
{
    Py::Dict ret = SolverBackend::Prefs::getPyValue();
    ret["minStep"] = Py::Float(minStep);
    return ret;
}

void BFGS::Prefs::setAttr(std::string attrname, Py::Object value)
{
    if (attrname == "minStep") {
        this->minStep = Py::Float(value);
    }
    else {
        SolverBackend::Prefs::setAttr(attrname, value);
    }
}

double BFGS::lineSearch(HSubSystem sys, ValueSet& vals, const Eigen::VectorXd& dir)
{
    iterLog("    begin line search");
    double f1,f2,f3,alpha1,alpha2,alpha3,alphaStar;

    //convert dir to ValueSet for maxstep calculation
    HValueSet xdir = ValueSet::make(sys->params(), dir);

    double alphaMax = sys->maxStep(vals, *xdir);
    iterLog("      max step alpha = %f", alphaMax);

    //dir is defined on _params. But vals may be defined on wider set of params. So we make a local copy.
    Eigen::VectorXd x0 = *ValueSet::makeFrom(sys->params(), vals);

    //func to calc error at x = x0 + alpha * dir
    auto errAtAlpha = [&](double alpha) -> double {
        vals.paste(x0 + alpha * xdir);
        return sys->error(vals);
    };

    //Start at the initial position alpha1 = 0
    alpha1 = 0.;
    f1 = sys->error(vals);

    //Take a step of alpha2 = 1
    alpha2 = 0.9;
    f2 = errAtAlpha(alpha2);

    //Take a step of alpha3 = 2*alpha2
    alpha3 = alpha2*2;
    f3 = errAtAlpha(alpha3);

    //Now reduce or lengthen alpha2 and alpha3 until the minimum is
    //Bracketed by the triplet f1>f2<f3
    bool max_reached = false;
    while (f2 > f1 || f2 > f3) {
        iterLog("      aplha1 = %f, alpha2 = %f, alpha3 = %f", alpha1, alpha2, alpha3);
        iterLog("      err1   = %f, err2   = %f, err3   = %f", f1,f2,f3);
        if (f2 > f1) {
            iterLog("      too far, retreating");
            //If f2 is greater than f1 then we shorten alpha2 and alpha3 closer to f1
            //Effectively both are shortened by a factor of two.
            alpha3 = alpha2;
            f3 = f2;
            alpha2 = alpha2 / 2;
            f2 = errAtAlpha(alpha2);
        }
        else if (f2 > f3) {
            iterLog("      too close, advancing");
            if (alpha3 >= alphaMax){
                iterLog("      can't advance, maxStep reached");
                max_reached = true;
                break;
            }
            //If f2 is greater than f3 then we increase alpha2 and alpha3 away from f1
            //Effectively both are lengthened by a factor of two.
            alpha2 = alpha3;
            f2 = f3;
            alpha3 = alpha3 * 2;
            f3 = errAtAlpha(alpha3);
        }
    }
    if (! max_reached){
        iterLog("      minimum is enclosed");
        iterLog("      aplha1 = %f, alpha2 = %f, alpha3 = %f", alpha1, alpha2, alpha3);
        iterLog("      err1   = %f, err2   = %f, err3   = %f", f1, f2, f3);
    }
    //Get the alpha for the minimum f of the quadratic approximation
    alphaStar = alpha2 + ((alpha2-alpha1)*(f1-f3))/(3*(f1-2*f2+f3));
    iterLog("      quadratic approximation minimum at alpha = %f", alphaStar);

    //Guarantee that the new alphaStar is within the bracket
    if (alphaStar >= alpha3 || alphaStar <= alpha1)
        alphaStar = alpha2;

    if (alphaStar > alphaMax)
        alphaStar = alphaMax;

    if (alphaStar != alphaStar)
        alphaStar = 0.;

    iterLog("    line search finished, alpha = %f", alphaStar);

    //Take a final step to alphaStar
    vals.paste(x0 + alphaStar * xdir);

    return alphaStar;

}

eSolveResult BFGS::solve(HSubSystem sys, HValueSet vals)
{
    iterLog("Begin BFGS solving");

    if (sys->isTouched())
        sys->update();

    int xsize = sys->params()->size();
    int csize = sys->subconstraints().size();

    if(xsize == 0 || csize == 0){
        iterLog("    nothing to do, finished.");
        return eSolveResult::Success;
    }

    sys->checkValuesCoverage(*vals);

    ssize_t maxIterNumber = _prefs.iterLimit(sys);

    Eigen::MatrixXd D //approximation of inverse of hessian matrix
            = Eigen::MatrixXd::Identity(xsize, xsize);
    HValueSet xdir = ValueSet::makeZeros(sys->params());
    Eigen::VectorXd grad(xsize);
    Eigen::VectorXd Dy(xsize);

    HValueSet h = ValueSet::makeZeros(sys->params()); //x step of an iteration


    sys->calcGrad(vals, grad);

    *xdir = -grad;

    double err = sys->error(*vals);
    double divergingLim = 1e6*err + 1e12;

    for (int iter = 0; iter < maxIterNumber; ++iter) {
        iterLog("  iteration %i", iter);
        iterLog("    sq(err) = %f", err);

        HValueSet x_old = ValueSet::makeFrom(sys->params(), *vals);
        lineSearch(sys, *vals, xdir->values());
        double err = sys->error(*vals);
        iterLog("    after line search: sq(err) = %f", err);

        *h = ValueSet::makeFrom(sys->params(), *vals) - *x_old; //=x-xold

        double h_norm = h->values().norm();
        iterLog("    line search step = %f", h_norm);

        if (h_norm <= (_prefs.minStep)){
            iterLog("BFGS solver converged (iteration step too small)");
            return eSolveResult::Minimized;
        }
        if (err != err){
            throw SolverError("Error function returned NaN");
        }
        if (err <= sq(_prefs.errorForSolved)){
            iterLog("    error is small, solved");
            return eSolveResult::Success;
        }
        if (err > divergingLim){
            throw SolverError("BFGS solver failed: diverged");
        }

        Eigen::VectorXd grad_old = grad;
        sys->calcGrad(vals, grad);
        Eigen::VectorXd y = grad - grad_old;

        double hty = h->values().dot(y);
        //make sure that hty is never 0
        if (fabs(hty) < 1e-80)
            hty = 1e-10;

        Dy = D * y;
        double ytDy = y.dot(Dy);
        iterLog("    h * d_grad = %f, d_grad * D * d_grad = %f", hty, ytDy);
        //Now calculate the BFGS update on D
        D += (1.+ytDy/hty)/hty * h->values() * h->values().transpose();
        D -= 1./hty * (h->values() * Dy.transpose() + Dy * h->values().transpose());

        *xdir = -D * grad;
    }
    throw SolverError("BFGS solver failed: iteration limit reached");
}

eSolveResult BFGS::solvePair(HSubSystem mainsys, HSubSystem auxsys, HValueSet vals)
{
    //not supported, redirect to main solve()
    return SolverBackend::solvePair(mainsys, auxsys, vals);
}
