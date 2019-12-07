#include "PreCompiled.h"

#include "DogLeg.h"
#include "DualMath.h"
#include "PyUtils.h"

#include "Eigen/Dense"

TYPESYSTEM_SOURCE(FCS::DogLeg, FCS::SolverBackend);

using namespace FCS;


Py::Dict DogLeg::Prefs::getPyValue() const
{
    Py::Dict ret = SolverBackend::Prefs::getPyValue();
    ret["minGrad"] = Py::Float(minGrad);
    ret["minStep"] = Py::Float(minStep);
    const char* gaussStepStrings[] = {"FullPivLU", "LeastNormFullPivLU", "LeastNormLdlt", nullptr};
    ret[""] = Py::String(gaussStepStrings[int(this->dogLegGaussStep)]);
    ret["initialTrustRegion"] = Py::Float(initialTrustRegion);
    ret["trustRegionExpandLinearityTolerance"] = Py::Float(trustRegionExpandLinearityTolerance);
    ret["trustRegionExpandMinStepSpan"] = Py::Float(trustRegionExpandMinStepSpan);
    ret["trustRegionExpandFactor"] = Py::Float(trustRegionExpandFactor);
    ret["trustRegionShrinkLinearityThreshold"] = Py::Float(trustRegionShrinkLinearityThreshold);
    ret["trustRegionShrinkFactor"] = Py::Float(trustRegionShrinkFactor);
    ret["trustRegionShrinkSpeedupFactor"] = Py::Float(trustRegionShrinkSpeedupFactor);
    return ret;
}

void DogLeg::Prefs::setAttr(std::string attrname, Py::Object value)
{
    if (attrname == "minGrad") {
        this->minGrad = Py::Float(value);
    }
    else if (attrname == "minStep") {
        this->minStep = Py::Float(value);
    }
    else if (attrname == "dogLegGaussStep") {
        const char* gaussStepStrings[] = {"FullPivLU", "LeastNormFullPivLU", "LeastNormLdlt", nullptr};
        this->dogLegGaussStep = str2enum<eDogLegGaussStep>(value, gaussStepStrings);
    }
    else if (attrname == "initialTrustRegion") {
        this->initialTrustRegion = Py::Float(value);
    }
    else if (attrname == "trustRegionExpandLinearityTolerance") {
        this->trustRegionExpandLinearityTolerance = Py::Float(value);
    }
    else if (attrname == "trustRegionExpandMinStepSpan") {
        this->trustRegionExpandMinStepSpan = Py::Float(value);
    }
    else if (attrname == "trustRegionExpandFactor") {
        this->trustRegionExpandFactor = Py::Float(value);
    }
    else if (attrname == "trustRegionShrinkLinearityThreshold") {
        this->trustRegionShrinkLinearityThreshold = Py::Float(value);
    }
    else if (attrname == "trustRegionShrinkFactor") {
        this->trustRegionShrinkFactor = Py::Float(value);
    }
    else if (attrname == "trustRegionShrinkSpeedupFactor") {
        this->trustRegionShrinkSpeedupFactor = Py::Float(value);
    }
    else {
        SolverBackend::Prefs::setAttr(attrname, value);
    }
}

eSolveResult DogLeg::solve(HSubSystem sys, HValueSet vals)
{
    iterLog("Begin Dogleg solving");

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

    HValueSet x_new = ValueSet::makeZeros(sys->params());
    Eigen::VectorXd fx(csize), //error values
                    fx_new(csize);
    Eigen::MatrixXd Jx(csize, xsize), Jx_new(csize, xsize);
    Eigen::VectorXd g(xsize),
            h_sd(xsize), //steepest descent xstep
            h_gn(xsize), //gauss-newton xstep
            h_dl(xsize); //actual xstep

    double err; // net qsuare error (times 0.5)
    HValueSet x = ValueSet::makeFrom(sys->params(), *vals);
    sys->calcResidual(*vals, fx, err);
    sys->calcJacobi(*vals, sys->params()->self(), Jx);

    g = Jx.transpose()*(-fx); //gradient?

    // get the max values from g and fx
    double g_inf = g.lpNorm<Eigen::Infinity>();
    double fx_inf = fx.lpNorm<Eigen::Infinity>();

    double divergingLim = 1e6*err + 1e12;

    double delta=_prefs.initialTrustRegion; //trust region radius (in parameter space)
    double nu=_prefs.trustRegionShrinkFactor;// by how much to divide trust region if linearity is bad
    ssize_t iter=0;
    int stop=0; //trop code; a flag to stop the loop
    int reduce=0; //flag: for how many next iterations the trust region should not be allowed to grow
    while (!stop) {
        iterLog("  new iteration");
        iterLog("    err = %f", err);
        // check if finished
        if (fx_inf <= _prefs.errorForSolved){ // Success
            stop = 1;
            break;
        } else if (g_inf <= _prefs.minGrad) {
            iterLog("gradient is small, probably stuck in a minimum");
            stop = 2;
            break;
        } else if (delta <= _prefs.minStep*(_prefs.minStep + x->values().norm())) {
            throw SolverError("Dogleg solver failed, because trust region is too small.");
        } else if (iter >= maxIterNumber) {
            throw SolverError("Dogleg solver failed, iteration limit exhausted.");
        } else if (err > divergingLim) {
            throw SolverError("Dogleg solver failed, diverged.");
        } else if (err != err) { // check for NaN
            throw SolverError("Dogleg solver failed, NaN error returned.");
        }


        { //compute the step
            // get the steepest descent direction
            double alpha = g.squaredNorm()/(Jx*g).squaredNorm();
            h_sd  = alpha*g;

            // get the gauss-newton step
            // http://forum.freecadweb.org/viewtopic.php?f=10&t=12769&start=50#p106220
            // https://forum.kde.org/viewtopic.php?f=74&t=129439#p346104
            switch (_prefs.dogLegGaussStep){
                case Prefs::eDogLegGaussStep::FullPivLU:
                    h_gn = Jx.fullPivLu().solve(-fx);
                break;
                case Prefs::eDogLegGaussStep::LeastNormFullPivLU:
                    h_gn = Jx.adjoint()*(Jx*Jx.adjoint()).fullPivLu().solve(-fx);
                break;
                case Prefs::eDogLegGaussStep::LeastNormLdlt:
                    h_gn = Jx.adjoint()*(Jx*Jx.adjoint()).ldlt().solve(-fx);
                break;
            }

            //reverse-engineered: (DeepSOIC)
            //assuming the system is linear, is doing the step we just calculated going to massively worsen the error?
            double rel_error = (Jx*h_gn + fx).norm() / fx.norm();
            if (rel_error > 1e15){
                throw SolverError("Dogleg solver failed, bad gauss-newton step (probably caused by nearly-singular matrix).");
            }

            // compute the dogleg step
            if (h_gn.norm() < delta) {
                iterLog("    gauss-newton step is within trust region "
                        "-> use it as the step");
                h_dl = h_gn;
                if  (h_dl.norm() <= _prefs.minStep*(_prefs.minStep + x->values().norm())) {
                    iterLog("    step is zero. Probably stuck in a minimum.");
                    stop = 2;
                    break;
                }
            }
            else if (h_sd.norm() >= delta) {
                //gauss-newton step is outside trust region
                //steepest-descent step is outside of trust region
                //-> constrain the descent step and use it
                iterLog("    gauss-newton step and gradient step exceed trust region. "
                      "\n    -> using gradient step constrained into trust region.");
                h_dl = h_sd * (delta/(h_sd.norm()));
            }
            else {
                //gauss-newton step is outside trust region
                //steepest-descent step is within trust region
                //compute beta
                double beta = 0; //beta is the merge factor of GN step and SD step. beta=0.0 means SD step, beta=1.0 means GN step.
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
                iterLog("    gauss-newton step exceeds trust region, but gradient step doesn't. "
                      "\n    -> using mix (%f) of the two.", beta);
            }
        }

        // it didn't work in some tests
        //   // restrict h_dl according to maxStep
        //   double scale = subsys->maxStep(h_dl);
        //   if (scale < 1.)
        //       h_dl *= scale;

        // do the step and compute the new error
        double err_new;
        *x_new = x + h_dl;
        vals->paste(x_new);
        sys->calcResidual(*vals, fx_new, err_new);
        sys->calcJacobi(*vals, sys->params()->self(), Jx_new);

        // calculate the linear model and the update ratio
        double dL = err - 0.5*(fx + Jx*h_dl).squaredNorm(); //linear expectation of error reduction
        double dF = err - err_new; //actual error reduction
        double rho = dF/dL;//accuracy of model: rho -> 1.0 for linear system

        iterLog("    trust region = %f", delta);
        iterLog("    GN step norm = %f", h_gn.norm());
        iterLog("    SD step norm = %f", h_sd.norm());
        iterLog("    dogleg step norm = %f", h_dl.norm());
        iterLog("    err_new = %f", err_new);
        iterLog("    linearity factor = %f", rho);

        if (dF > 0 && dL > 0) {
            //errors reduced, great! apply the step
            *x = *x_new;
            Jx = Jx_new;
            fx = fx_new;
            err = err_new;

            g = Jx.transpose()*(-fx);

            // get infinity norms
            g_inf = g.lpNorm<Eigen::Infinity>();
            fx_inf = fx.lpNorm<Eigen::Infinity>();
        }
        else {
            iterLog("    error was not reduced, rejecting the step.");
            rho = -1;
        }

        // update trust region
        //if (linear approximation is decent) and (the step we are taking is substantial compared to trust region size) and (flag to reduce trust region wasn't set)
        if (fabs(rho-1.0) < _prefs.trustRegionExpandLinearityTolerance
            && h_dl.norm() > delta * _prefs.trustRegionExpandMinStepSpan
            && reduce <= 0) {
            iterLog("    expanding trust region");
            delta *= _prefs.trustRegionExpandFactor;
            nu = _prefs.trustRegionShrinkFactor;
            reduce = 0;
        }
        else if (rho < _prefs.trustRegionShrinkLinearityThreshold) {
            //linear model was poor, error reduction  seriously below expectation.
            //->reduce trust region
            iterLog("    shrinking trust region");
            delta = delta/nu;
            nu *= _prefs.trustRegionShrinkSpeedupFactor;//if on the next step it is still poor, reduce it even harder
            reduce = 2;
        }
        else {

            // linear model was poor, but improvement was bigger than expected
            //or
            // ok linearity, but step is significantly smaller than trust region
            //or
            // we would have let the trust region to grow, but the flag to reduce trust region was set
            //->do not change trust region
            --reduce;
            iterLog("    (trust region unchanged)");
        }

        ++iter;

    }

    if (stop == 1)
        iterLog("solved");
    else
        iterLog("minimized");

    return (stop == 1) ? eSolveResult::Success : eSolveResult::Minimized;
}

eSolveResult DogLeg::solvePair(HSubSystem mainsys, HSubSystem auxsys, HValueSet vals)
{
    //not supported, redirect to main solve()
    return SolverBackend::solvePair(mainsys, auxsys, vals);
}
