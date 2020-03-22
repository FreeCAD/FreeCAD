#include "PreCompiled.h"

#include "SubSystem.h"
#include "SubSystemPy.h"

#include <Base/DualNumber.h>
#include "DualMath.h"

using namespace FCS;
using DualNumber = Base::DualNumber;

TYPESYSTEM_SOURCE(FCS::SubSystem, Base::BaseClass);


SubSystem::SubSystem()
    : _store(Py::None()), _params(Py::None()), _curvals(Py::None())
{

}

SubSystem::SubSystem(HParameterSubset params, std::vector<HConstraint> constraints)
    : _store(params->host()), _params(params), _curvals(Py::None())
{
    _constraints = constraints;
    update();
}

void SubSystem::calcJacobi(ValueSet& vals, HParameterSubset params, Eigen::MatrixXd& output)
{
    if (_touched)
        update();

    vals.resetDerivatives();
    output.setZero(_subconstraints.size(), params->size());

    std::vector<DualNumber> buf (_maxConstraintRank); //buffer that receives constraint error values
    //indexes:
    // * ic: index of (multidimensional) constraint, into _constraints
    // * ie: index of error value in error vector of _constraints[ic]
    // * ir + ie: index of subconstraint, aka row index
    // * ip: index of parameter in params
    for(size_t ic = 0; ic < _constraints.size(); ++ic){
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
            //assert(ip != -1); //#FIXME: if the assert is never hit, remove the following "if".
            if (ip != -1){ //same as "if vals->subset().has(p)"
                // set dual part of parameter to 1 for deriv computation
                vals.setDual_solver(p, 1.0);
                //compute error. Derivatives are in dual parts, error values are discarded.
                constr.error(vals, buf.data());
                //write results to matrix
                for(int ie = 0; ie < rank; ++ie){
                    //row = index of subconstraint
                    //col = index of parameter
                    output(ir+ie, ip) = buf[ie].du * constr.scale;
                }
                //reset back, to make sure all duals are zero again as we move on to next parameter
                vals.setDual_solver(p, 0.0);
            }
        }
    }
}

void SubSystem::calcJacobi(Eigen::MatrixXd& output)
{
    if (_touched)
        update();

    HValueSet vals = _curvals;
    if (vals.isNone()){
        vals = ValueSet::make(_params);
    }
    calcJacobi(*vals, _params, output);
}

void SubSystem::calcGrad(HValueSet vals, Eigen::VectorXd& output)
{
    if (_touched)
        update();

    vals->resetDerivatives();
    assert(output.size() == _params->size());
    output.setZero();

    std::vector<DualNumber> buf (_maxConstraintRank); //buffer that receives constraint error values
    //indexes:
    // * ic: index of (multidimensional) constraint, into _constraints
    // * ie: index of error value in error vector of _constraints[ic]
    // * ir + ie: index of subconstraint, aka row index
    // * ip: index of parameter in vals
    for(size_t ic = 0; ic < _constraints.size(); ++ic){
        Constraint& constr = *(_constraints[ic]);
        int rank = constr.rank();
        //int ir = _constraint1stRow[ic]; //unused
        for(const ParameterRef& p : constr.parameters()){
            //calculate derivatives of this constraint error function
            //on parameter p

            int ip = _params->indexOf(p);

            //since we compute jacobi for arbitrary set of parameters, we can't
            //assume all parameters the constraint depends on are in vals.
            //Though, in practice, they should always be. But check anyway.
            //assert(ip != -1); //#FIXME: if the assert is never hit, remove the following "if".
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
    if (_touched)
        update();

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
    if (_touched)
        update();

    assert(output.size() == _subconstraints.size());

    std::vector<DualNumber> buf (_maxConstraintRank); //buffer that receives constraint error values

    err = 0;
    int ir = 0;
    for(size_t ic = 0; ic < _constraints.size(); ++ic){
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
    if (_touched)
        update();
    Eigen::VectorXd _(_subconstraints.size());
    double err = 0;
    calcResidual(vals, _, err);
    return err;
}

void SubSystem::checkValuesCoverage(const ValueSet& vals) const
{
    if (!_params->in(vals.subset().self()))
        throw Py::ValueError("ValueSet doesn't contain all the parameters of the subsystem, can't calculate derivatives");
}

PyObject* SubSystem::getPyObject()
{
    if (_twin == nullptr){
        new SubSystemPy(this);
        assert(_twin);
        return _twin;
    } else {
        return Py::new_reference_to(_twin);
    }
}

HSubSystem SubSystem::self()
{
    return HSubSystem(getPyObject(), true);
}

void SubSystem::update()
{
    if (_store.isNone() || _params.isNone())
        throw Base::ReferencesError("SubSystem is not initialized");
    _subconstraints.clear();
    _constraint1stRow.resize(_constraints.size());
    _maxConstraintRank = 0;
    int isc = 0;//index of subconstraint
    for(size_t ic = 0; ic <  _constraints.size(); ++ic){
        HConstraint& c = _constraints[ic];
        _constraint1stRow[ic] = isc;
        for(int ie = 0; ie < c->rank(); ++ie){
            _subconstraints.push_back(Subconstraint{c, ie});
            ++isc;
        }
        if (c->rank() > _maxConstraintRank)
            _maxConstraintRank = c->rank();
    }
    _touched = false;
}

void SubSystem::addConstraint(HConstraint c)
{
    _constraints.push_back(c);
    touch();
}

void SubSystem::addConstraint(const std::vector<HConstraint>& clist)
{
    for (const HConstraint& c : clist)
        addConstraint(c);
}

void SubSystem::addUnknown(const ParameterRef &p)
{
    if (_store.isNone())
        _store = p.host();
    if (_params.isNone())
        _params = ParameterSubset::make(_store);
    _params->add(p);
}

void SubSystem::addUnknown(HParameterSubset subset)
{
    if (_params.isNone() || _params->size() == 0){
        _params = subset;
        _store = _params->host();
    } else {
        for (const ParameterRef& p : subset->list())
            addUnknown(p);
    }
}

