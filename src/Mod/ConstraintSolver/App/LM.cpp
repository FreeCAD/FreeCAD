#include "PreCompiled.h"

#include "LM.h"
#include "DualMath.h"

#include "Eigen/Dense"
#include <cfloat> //for DBL_EPSILON

TYPESYSTEM_SOURCE(FCS::LM, FCS::SolverBackend);

using namespace FCS;


Py::Dict LM::Prefs::getPyValue() const
{
    Py::Dict ret = SolverBackend::Prefs::getPyValue();
    ret["minGrad"] = Py::Float(minGrad);
    ret["initialDampingFactor"] = Py::Float(initialDampingFactor);
    ret["dampingFactorBoostMultiplier"] = Py::Float(dampingFactorBoostMultiplier);
    ret["dampingFactorBoostSpeedupMultiplier"] = Py::Float(dampingFactorBoostSpeedupMultiplier);
    ret["dampingFactorReductionMultiplier"] = Py::Float(dampingFactorReductionMultiplier);
    return ret;
}

void LM::Prefs::setAttr(std::string attrname, Py::Object value)
{
    if (attrname == "minGrad") {
        this->minGrad = Py::Float(value);
    }
    else if (attrname == "initialDampingFactor") {
        this->initialDampingFactor = Py::Float(value);
    }
    else if (attrname == "dampingFactorBoostMultiplier") {
        this->dampingFactorBoostMultiplier = Py::Float(value);
    }
    else if (attrname == "dampingFactorBoostSpeedupMultiplier") {
        this->dampingFactorBoostSpeedupMultiplier = Py::Float(value);
    }
    else if (attrname == "dampingFactorReductionMultiplier") {
        this->dampingFactorReductionMultiplier = Py::Float(value);
    }
    else {
        SolverBackend::Prefs::setAttr(attrname, value);
    }
}

eSolveResult LM::solve(HSubSystem sys, HValueSet vals)
{
    iterLog("Begin Levenberg-Marquardt solving");

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

    Eigen::VectorXd e(csize), e_new(csize); // vector of all constraint errors
    Eigen::MatrixXd J(csize, xsize);        // Jacobi of the subsystem
    Eigen::MatrixXd A(xsize, xsize);        //J^T * J,
    HValueSet h = ValueSet::makeZeros(sys->params()); //x step of an iteration
    Eigen::VectorXd g(xsize), //gradient
                    diag_A(xsize); //saved diag elements (to undo damping)

    HValueSet x = ValueSet::makeFrom(sys->params(), *vals);
    HValueSet x_new = x->copy();

    double _;
    sys->calcResidual(*vals, e, _);
    e *= -1.0;

    double divergingLim = 1e6*e.squaredNorm() + 1e12;

    double nu = _prefs.dampingFactorBoostMultiplier; //booster multiplier for damping factor, in case current doesn't improve the error
    double mu = 0; //current damping factor (scaled to be ~A)
    int iter=0, stop=0;
    for (iter=0; iter < maxIterNumber && !stop; ++iter) {
        iterLog("  iteration %i", iter);
        double err = e.squaredNorm();
        iterLog("    err = %f", sqrt(err));
        if (err <= sq(_prefs.errorForSolved)) {
            iterLog("error is small, solved");
            stop = 1;
            break;
        } else if (err > divergingLim) {
            throw SolverError("LM solver failed: diverged.");
        } else if (err != err) { // check for NaN
            throw SolverError("LM solver failed: NaN error returned.");
        }

        sys->calcJacobi(*vals, sys->params(), J);

        A = J.transpose()*J; //(xsize by xsize matrix)
        g = J.transpose()*e; //gradient

        //max absolute of gradient's components
        double g_inf = g.lpNorm<Eigen::Infinity>();
        // check for convergence
        if (g_inf <= _prefs.minGrad) {
            iterLog("    gradient is zero (stuck in a local minimum).");
            stop = 2;
            break;
        }
        iterLog("    max(abs(gradient[i])) = %f", g_inf);

        diag_A = A.diagonal(); // save diagonal entries so that augmentation can be later canceled
        // compute initial damping factor
        if (iter == 0)
            mu = _prefs.initialDampingFactor * diag_A.lpNorm<Eigen::Infinity>();


        double h_norm = std::numeric_limits<double>::quiet_NaN() ;
        // determine increment using adaptive damping
        int k=0;
        while (k < 50) {
            iterLog("    damping factor = %f, computing step...", mu);
            // augment normal equations A = A+uI
            for (int i=0; i < xsize; ++i)
                A(i,i) += mu;

            //solve augmented functions A*h=g. This is equivalent to J*h = e, if J is invertable and no damping was applied
            *h = A.fullPivLu().solve(g);

            // check if solving works
            double rel_error = (A*h->values() - g).norm() / g.norm();
            if (rel_error < 1e-5) {

                // restrict h according to maxStep
                double hscale = sys->maxStep(*vals, *h);
                if (hscale < 1.){
                    iterLog("    step limited by a factor of %f", hscale);
                    h->values() *= hscale;
                }

                h_norm = h->values().squaredNorm();
                iterLog("    sq(step) = %f", h_norm);

                if (h_norm <= _prefs.minGrad * _prefs.minGrad * x->values().norm()) { // relative change in p is small, stop
                    throw SolverError("LM solver failed:  step is too small.");
                }
                else if (h_norm >= (x->values().norm()+_prefs.minGrad)/(DBL_EPSILON*DBL_EPSILON)) { // almost singular
                    throw SolverError("LM solver failed: very large step - nearly singular matrix.");
                }


                // compute at new position
                *x_new = x + h;
                vals->paste(x_new);
                sys->calcResidual(*vals, e_new, _);
                e_new *= -1;

                double dF = e.squaredNorm() - e_new.squaredNorm();
                double dL = h->values().dot(mu * h->values() + g);

                iterLog("    step changed the error by %f (expected: %f)", dF, dL);

                if (dF>0. && dL>0.) { // reduction in error, increment is accepted
                    iterLog("    step accepted");

                    double tmp=2*dF/dL-1.;
                    double mumult = std::max(_prefs.dampingFactorReductionMultiplier,
                                             1.-tmp*tmp*tmp); //If df == dl or better, divide mu by 3. If df is very small, multiply by 2. At df = 0.5*dl, the function has a plateu, with a value of 1.0.
                    mu *= mumult; //change damping
                    iterLog("    changing damping by factor %f", mumult);
                    nu=2;

                    // accept new parameter values
                    *x = x_new;
                    e = e_new;
                    break;
                }
            } else {
                iterLog("    dampened linearized system yielded poor solution");
            }

            // if this point is reached, either the linear system could not be solved or
            // the error did not reduce; in any case, the increment must be rejected

            iterLog("    step does not reduce the error, retreating");

            iterLog("    boosting damping by %f", nu);
            mu*=nu; //increase damping
            nu*=_prefs.dampingFactorBoostSpeedupMultiplier; //and set to increase the damping even harder if this happens again
            for (int i=0; i < xsize; ++i) // restore diagonal J^T J entries
                A(i,i) = diag_A(i);

            k++;

        }
        if (k == 50) {
            throw SolverError("LM solver failed: Increasing damping did not induce convergence.");
        }
    } //iteration loop

    if (iter >= maxIterNumber){
        throw SolverError("LM solver failed: iteration limit reached.");
    }

    return (stop == 1) ? eSolveResult::Success : eSolveResult::Minimized;

}

eSolveResult LM::solvePair(HSubSystem mainsys, HSubSystem auxsys, HValueSet vals)
{
    //not supported, redirect to main solve()
    return SolverBackend::solvePair(mainsys, auxsys, vals);
}
