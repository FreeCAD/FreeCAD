#include "PreCompiled.h"

#include "SketchSolverSystem.h"

#include <Base/DualNumber.h>
#include "DualMath.h"

using namespace FCS;
using DualNumber = Base::DualNumber;


SubSystem::SubSystem(HParameterSubset params, std::vector<HConstraint> constraints)
    : _curvals(Py::None()), _store(params->host()), _params(params)
{
    _constraints = constraints;
    update();
}

void SubSystem::calcJacobi(ValueSet& vals, HParameterSubset params, Eigen::MatrixXd& output)
{
    vals.resetDerivatives();
    output.setZero(_subconstraints.size(), params->size());

    std::vector<DualNumber> buf (_maxConstraintRank); //buffer that receives constraint error values
    //indexes:
    // * ic: index of (multidimensional) constraint, into _constraints
    // * ie: index of error value in error vector if _constraints[ic]
    // * ic + ie: index of subconstraint, aka row index
    // * ip: index of parameter in params
    for(int ic = 0; ic < _constraints.size(); ++ic){
        Constraint& constr = *(_constraints[ic]);
        int rank = constr.rank();
        int ir = _constraint1stRow[ic];
        for(const ParameterRef& p : constr.parameters()){
            //calculate derivatives of this constraint error function
            //on parameter p

            int ip = params->indexOf(p);

            //since we compute jacobi for arbitrary set of parameters, we can't
            //assume all parameters the constraint depends on are in params.
            //Though, in practice, they should always be. But check anyway.
            assert(ip != -1); //#FIXME: if the assert is never hit, remove the following "if".
            if (ip != -1){ //same as "if vals->subset().has(p)"
                // set dual part of parameter to 1 for deriv computation
                vals.setDual(p, 1.0);
                //compute error. Derivatives are in dual parts, error values are discarded.
                constr.error(vals, buf.data());
                //write results to matrix
                for(int ie = 0; ie < rank; ++ie){
                    //row = index of subconstraint
                    //col = index of parameter
                    output(ir+ie, ip) = buf[ie].du * constr.scale;
                }
                //reset back, to make sure all duals are zero again as we move on to next parameter
                vals.setDual(p, 0.0);
            }
        }
    }
}

void SubSystem::calcJacobi(Eigen::MatrixXd& output)
{
    HValueSet vals = _curvals;
    if (vals.isNone()){
        vals = ValueSet::make(_params);
    }
    calcJacobi(*vals, _params, output);
}

void SubSystem::calcGrad(HValueSet vals, Eigen::VectorXd& output)
{
    vals->resetDerivatives();
    assert(output.size() == vals->size());
    output.setZero();

    std::vector<DualNumber> buf (_maxConstraintRank); //buffer that receives constraint error values
    //indexes:
    // * ic: index of (multidimensional) constraint, into _constraints
    // * ie: index of error value in error vector of _constraints[ic]
    // * ic + ie: index of subconstraint, aka row index
    // * ip: index of parameter in vals
    for(int ic = 0; ic < _constraints.size(); ++ic){
        Constraint& constr = *(_constraints[ic]);
        int rank = constr.rank();
        int ir = _constraint1stRow[ic];
        for(const ParameterRef& p : constr.parameters()){
            //calculate derivatives of this constraint error function
            //on parameter p

            int ip = vals->subset().indexOf(p);

            //since we compute jacobi for arbitrary set of parameters, we can't
            //assume all parameters the constraint depends on are in vals.
            //Though, in practice, they should always be. But check anyway.
            assert(ip != -1); //#FIXME: if the assert is never hit, remove the following "if".
            if (ip != -1){ //same as "if vals->subset().has(p)"
                // set dual part of parameter to 1 for deriv computation
                vals->duals()[ip] = 1.0;
                //compute error. Derivatives are in dual parts, error values are discarded.
                constr.error(*vals, buf.data());
                //write results to matrix
                for(int ie = 0; ie < rank; ++ie){
                    output(ip) += buf[ie].du * constr.scale * buf[ie].re * constr.scale;
                    //this addition comes from derivative of sum of squares of errors.
                }
                //reset back, to make sure all duals are zero again as we move on to next parameter
                vals->duals()[ip] = 0.0;
            }
        }
    }

}

double SubSystem::maxStep(const ValueSet& vals, const ValueSet& xdir)
{
    double step = 1e12;
    for(HConstraint hc : _constraints){
        step = std::min(step,
            hc->maxStep(vals, xdir)
        );
    }
    return step;
}

void SubSystem::calcResidual(const ValueSet& vals, Eigen::VectorXd &output, double& err)
{
    assert(output.size() == _subconstraints.size());

    std::vector<DualNumber> buf (_maxConstraintRank); //buffer that receives constraint error values

    err = 0;
    int ir = 0;
    for(int ic = 0; ic < _constraints.size(); ++ic){
        Constraint& constr = *(_constraints[ic]);
        int rank = constr.rank();
        constr.error(vals, buf.data());
        for(int ie = 0; ie < rank; ++ie){
            output(ir) = buf[ie].re * constr.scale;
            err += sq(output(ir));
            ++ir;
        }
    }
    err *= 0.5;// why? --DeepSOIC
}

double SubSystem::error(const ValueSet& vals)
{
    Eigen::VectorXd _(_subconstraints.size());
    double err = 0;
    calcResidual(vals, _, err);
    return err;
}

double SubSystem::lineSearch(ValueSet& vals, const Eigen::VectorXd& dir)
{
    double f1,f2,f3,alpha1,alpha2,alpha3,alphaStar;

    //convert dir to ValueSet
    HValueSet xdir = ValueSet::make(_params, vals);

    double alphaMax = maxStep(vals, *xdir);

    //dir is defined on _params. But vals may be defined on wider set of params. So we make a local copy.
    Eigen::VectorXd& x0 = *ValueSet::makeFrom(_params, vals);

    //Start at the initial position alpha1 = 0
    alpha1 = 0.;
    f1 = error(vals);

    //Take a step of alpha2 = 1
    alpha2 = 1.;
    vals.paste(x0 + alpha2 * xdir);
    f2 = error(vals);

    //Take a step of alpha3 = 2*alpha2
    alpha3 = alpha2*2;
    vals.paste(x0 + alpha3 * xdir);
    f3 = error(vals);

    //Now reduce or lengthen alpha2 and alpha3 until the minimum is
    //Bracketed by the triplet f1>f2<f3
    while (f2 > f1 || f2 > f3) {
        if (f2 > f1) {
            //If f2 is greater than f1 then we shorten alpha2 and alpha3 closer to f1
            //Effectively both are shortened by a factor of two.
            alpha3 = alpha2;
            f3 = f2;
            alpha2 = alpha2 / 2;
            vals.paste(x0 + alpha2 * xdir);
            f2 = error(vals);
        }
        else if (f2 > f3) {
            if (alpha3 >= alphaMax)
                break;
            //If f2 is greater than f3 then we increase alpha2 and alpha3 away from f1
            //Effectively both are lengthened by a factor of two.
            alpha2 = alpha3;
            f2 = f3;
            alpha3 = alpha3 * 2;
            vals.paste(x0 + alpha3 * xdir);
            f3 = error(vals);
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
    vals.paste(x0 + alphaStar * xdir);

    return alphaStar;

}

void SubSystem::update()
{
    _subconstraints.clear();
    _constraint1stRow.resize(_constraints.size());
    int isc = 0;//index of subconstraint
    for(int ic = 0; ic <  _constraints.size(); ++ic){
        HConstraint& c = _constraints[ic];
        _constraint1stRow[ic] = isc;
        for(int ie = 0; ie < c->rank(); ++ie){
            _subconstraints.push_back(Subconstraint{c, ie});
            ++isc;
        }
    }
}

