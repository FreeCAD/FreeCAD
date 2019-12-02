#include "PreCompiled.h"

#include "SketchSolver.h"
#include "SketchSolverPy.h"
#include "DualMath.h"

#include <Base/Console.h>

#include "Eigen/Dense"

TYPESYSTEM_SOURCE(FCS::SketchSolver, Base::BaseClass);

using namespace FCS;

template <typename... Args>
inline void SketchSolver::iterLog(std::string msg, Args... args)
{
    if(solverPrefs.debugMode == SolverPrefs::eDebugMode::IterationLevel) {
        Base::Console().Log(msg.c_str(), args...);
        Base::Console().Log("\n");
    }
}



SketchSolver::SketchSolver()
    :_fullSystem(Py::None())
{

}

SketchSolver::eSolveResult FCS::SketchSolver::solveDogLeg(FCS::HSubSystem sys, HValueSet vals, FCS::SketchSolver::DogLegPrefs prefs)
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

    ssize_t maxIterNumber = solverPrefs.iterLimit(sys);

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

    double delta=prefs.initialTrustRegion; //trust region radius (in parameter space)
    double nu=prefs.trustRegionShrinkFactor;// by how much to divide trust region if linearity is bad
    ssize_t iter=0;
    int stop=0; //trop code; a flag to stop the loop
    int reduce=0; //flag: for how many next iterations the trust region should not be allowed to grow
    while (!stop) {
        iterLog("  new iteration");
        iterLog("    err = %f", err);
        // check if finished
        if (fx_inf <= prefs.tolf){ // Success
            stop = 1;
            break;
        } else if (g_inf <= prefs.tolg) {
            //throw, maybe?
            Base::Console().Error("Dogleg solver failed, because gradient is too small.\n");
            stop = 2;
            break;
        } else if (delta <= prefs.tolx*(prefs.tolx + x->values().norm())) {
            Base::Console().Error("Dogleg solver failed, because trust region is too small.\n");
            stop = 2;
            break;
        } else if (iter >= maxIterNumber) {
            Base::Console().Error("Dogleg solver failed, iteration limit exhausted.\n");
            stop = 4;
            break;
        } else if (err > divergingLim) {
            Base::Console().Error("Dogleg solver failed, diverged.\n");
            stop = 6;
            break;
        } else if (err != err) { // check for NaN
            Base::Console().Error("Dogleg solver failed, NaN error returned.\n");
            stop = 6;
            break;
        }


        { //compute the step
            // get the steepest descent direction
            double alpha = g.squaredNorm()/(Jx*g).squaredNorm();
            h_sd  = alpha*g;

            // get the gauss-newton step
            // http://forum.freecadweb.org/viewtopic.php?f=10&t=12769&start=50#p106220
            // https://forum.kde.org/viewtopic.php?f=74&t=129439#p346104
            switch (prefs.dogLegGaussStep){
                case DogLegPrefs::eDogLegGaussStep::FullPivLU:
                    h_gn = Jx.fullPivLu().solve(-fx);
                break;
                case DogLegPrefs::eDogLegGaussStep::LeastNormFullPivLU:
                    h_gn = Jx.adjoint()*(Jx*Jx.adjoint()).fullPivLu().solve(-fx);
                break;
                case DogLegPrefs::eDogLegGaussStep::LeastNormLdlt:
                    h_gn = Jx.adjoint()*(Jx*Jx.adjoint()).ldlt().solve(-fx);
                break;
            }

            //reverse-engineered: (DeepSOIC)
            //assuming the system is linear, is doing the step we just calculated going to massively worsen the error?
            double rel_error = (Jx*h_gn + fx).norm() / fx.norm();
            if (rel_error > 1e15){
                Base::Console().Error("Dogleg solver failed, bad gauss-newton step (probably caused by nearly-singular matrix).\n");
                break;
            }

            // compute the dogleg step
            if (h_gn.norm() < delta) {
                iterLog("    gauss-newton step is within trust region "
                        "-> use it as the step");
                h_dl = h_gn;
                if  (h_dl.norm() <= prefs.tolx*(prefs.tolx + x->values().norm())) {
                    Base::Console().Error("Dogleg solver failed, zero gauss-newton step.\n");
                    stop = 5;
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
        if (fabs(rho-1.0) < prefs.trustRegionExpandLinearityTolerance
            && h_dl.norm() > delta * prefs.trustRegionExpandMinStepSpan
            && reduce <= 0) {
            iterLog("    expanding trust region");
            delta *= prefs.trustRegionExpandFactor;
            nu = prefs.trustRegionShrinkFactor;
            reduce = 0;
        }
        else if (rho < prefs.trustRegionShrinkLinearityThreshold) {
            //linear model was poor, error reduction  seriously below expectation.
            //->reduce trust region
            iterLog("    shrinking trust region");
            delta = delta/nu;
            nu *= prefs.trustRegionShrinkSpeedupFactor;//if on the next step it is still poor, reduce it even harder
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
        iterLog("dogleg solver failed");

    return (stop == 1) ? eSolveResult::Success : eSolveResult::Failed;
}

SketchSolver::eSolveResult SketchSolver::solveLM(HSubSystem sys, HValueSet vals, SketchSolver::LMPrefs prefs)
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

    ssize_t maxIterNumber = solverPrefs.iterLimit(sys);

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

    double nu = prefs.dampingFactorBoostMultiplier; //booster multiplier for damping factor, in case current doesn't improve the error
    double mu = 0; //current damping factor (scaled to be ~A)
    int iter=0, stop=0;
    for (iter=0; iter < maxIterNumber && !stop; ++iter) {
        iterLog("  iteration %i", iter);
        double err = e.squaredNorm();
        iterLog("    err = %f", sqrt(err));
        if (err <= sq(prefs.zeroError)) {
            iterLog("error is small, solved");
            stop = 1;
            break;
        } else if (err > divergingLim) {
            Base::Console().Error("LM solver failed: diverged.\n");
            stop = 6;
            break;
        } else if (err != err) { // check for NaN
            Base::Console().Error("LM solver failed: NaN error returned.\n");
            stop = 6;
            break;
        }

        sys->calcJacobi(*vals, sys->params(), J);

        A = J.transpose()*J; //(xsize by xsize matrix)
        g = J.transpose()*e; //gradient

        //max absolute of gradient's components
        double g_inf = g.lpNorm<Eigen::Infinity>();
        // check for convergence
        if (g_inf <= prefs.zeroGradient) {
            Base::Console().Error("LM solver failed: gradient is zero (stuck in a local minimum).\n");
            stop = 2;
            break;
        }
        iterLog("    max(abs(gradient[i])) = %f", g_inf);

        diag_A = A.diagonal(); // save diagonal entries so that augmentation can be later canceled
        // compute initial damping factor
        if (iter == 0)
            mu = prefs.initialDampingFactor * diag_A.lpNorm<Eigen::Infinity>();


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

                if (h_norm <= prefs.zeroGradient * prefs.zeroGradient * x->values().norm()) { // relative change in p is small, stop
                    Base::Console().Error("LM solver failed:  step is too small.\n");
                    stop = 3;
                    break;
                }
                else if (h_norm >= (x->values().norm()+prefs.zeroGradient)/(DBL_EPSILON*DBL_EPSILON)) { // almost singular
                    Base::Console().Error("LM solver failed: very large step - nearly singular matrix.\n");
                    stop = 4;
                    break;
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
                    double mumult = std::max(prefs.dampingFactorReductionMultiplier,
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
            nu*=prefs.dampingFactorBoostSpeedupMultiplier; //and set to increase the damping even harder if this happens again
            for (int i=0; i < xsize; ++i) // restore diagonal J^T J entries
                A(i,i) = diag_A(i);

            k++;

        }
        if (k == 50) {
            Base::Console().Error("LM solver failed: Increasing damping did not induce convergence.\n");
            stop = 7;
            break;
        }
    } //iteration loop

    if (iter >= maxIterNumber){
        Base::Console().Error("LM solver failed: iteration limit reached.\n");
        stop = 5;
    }

    return (stop == 1) ? eSolveResult::Success : eSolveResult::Failed;
}

double SketchSolver::lineSearch(HSubSystem sys, ValueSet& vals, const Eigen::VectorXd& dir)
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


SketchSolver::eSolveResult SketchSolver::solveBFGS(HSubSystem sys, HValueSet vals, SketchSolver::BFGSPrefs prefs)
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

    ssize_t maxIterNumber = solverPrefs.iterLimit(sys);

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

        if (h_norm <= (prefs.convergence)){
            Base::Console().Error("BFGS solver failed: stuck in a local minimum (iteration step too small)\n");
            return eSolveResult::Minimized;
        }
        if (err <= prefs.smallF){
            iterLog("    error is small, solved");
            return eSolveResult::Success;
        }
        if (err > divergingLim){
            Base::Console().Error("BFGS solver failed: diverged\n");
            return eSolveResult::Failed;
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
    Base::Console().Error("BFGS solver failed: iteration limit reached\n");
    return eSolveResult::Failed;

}

PyObject* SketchSolver::getPyObject()
{
    if (_twin == nullptr){
        new SketchSolverPy(this);
        assert(_twin);
        return _twin;
    } else {
        return Py::new_reference_to(_twin);
    }
}

HSketchSolver SketchSolver::self()
{
    return HSketchSolver(getPyObject(), true);
}

