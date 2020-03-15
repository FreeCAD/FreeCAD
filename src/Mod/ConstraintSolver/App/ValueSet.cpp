#include "PreCompiled.h"

#include "ValueSet.h"
#include "ValueSetPy.h"

using namespace FCS;
using DualNumer = Base::DualNumber;

FCS::ValueSet::ValueSet(FCS::HParameterSubset subset)
    : _subset(subset)
{
    init();
    reset();
}

void ValueSet::init(bool skip_values)
{
    if (! skip_values)
        _values.resize(size());
    _duals.resize(size(), 0.0);
    _scales.resize(size(), 0.0);
    for (int i = 0; i < size(); ++i) {
        _scales[i] = subset()[i].masterScale();
    }
}

void ValueSet::checkSameSize(int sz)
{
    if (sz != size()){
        std::stringstream ss;
        ss << "Length of argument vector (" << sz
           << ") doesn't match the number of parameters in ValueSet (" << size()
           << "). Use .paste or .initFrom methods instead.";
        throw Py::ValueError(ss.str());
    }
}

void ValueSet::checkSameSet(const ValueSet& other)
{
    if (! other._subset.is(_subset))
        throw Py::ValueError("Parameter sets of these value sets are not same. Use .paste or .initFrom methods to assign values.");
}

ValueSet::ValueSet(HParameterSubset subset, const Eigen::VectorXd& vals, bool no_size_check)
    : _subset(subset)
{
    if (!no_size_check){
        checkSameSize(vals.size());
    }
    init(/*skip_values = */true);
    _values = vals;
}

ValueSet::ValueSet(HParameterSubset subset, const ValueSet& other)
    : _subset(subset)
{
    init();
    initFrom(other);
}

HValueSet ValueSet::make(HParameterSubset subset)
{
    ValueSet* obj = new ValueSet(subset);
    PyObject* pyobj = new ValueSetPy(obj);
    obj->_twin = pyobj;
    return HValueSet(pyobj, /*new_reference=*/true);
}

HValueSet ValueSet::make(HParameterSubset subset, const Eigen::VectorXd& vals, bool no_size_check)
{
    ValueSet* obj = new ValueSet(subset, vals, no_size_check);
    PyObject* pyobj = new ValueSetPy(obj);
    obj->_twin = pyobj;
    return HValueSet(pyobj, /*new_reference=*/true);
}

HValueSet ValueSet::makeTrivial(HParameterStore store)
{
    HParameterSubset s = ParameterSubset::make(store);
    HValueSet vs = make(s);
    vs->_passthru = true;
    return vs;
}

HValueSet ValueSet::makeFrom(HParameterSubset subset, const ValueSet& other)
{
    ValueSet* obj = new ValueSet(subset, other);
    PyObject* pyobj = new ValueSetPy(obj);
    obj->_twin = pyobj;
    return HValueSet(pyobj, /*new_reference=*/true);
}

HValueSet ValueSet::makeZeros(HParameterSubset subset)
{
    return make(subset, Eigen::VectorXd(subset->size()));
}

HValueSet ValueSet::copy() const
{
    HValueSet cpy = make(this->_subset);
    cpy->_values = this->_values;
    cpy->_duals = this->_duals;
    cpy->_scales = this->_scales;
    cpy->_passthru = this->_passthru;
    return cpy;
}

int ValueSet::size() const
{
    return _subset->size();
}

const ParameterSubset& ValueSet::subset() const
{
    return *_subset;
}

Eigen::VectorXd ValueSet::savedValues() const
{
    Eigen::VectorXd ret(size());
    for (int i = 0; i < size(); ++i) {
        ParameterRef param = subset()[i];
        ret[i] = param.savedValue() / _scales[i];
    }
    return ret;
}

Eigen::VectorXd ValueSet::getSubvector(const ParameterSubset& params) const
{
    Eigen::VectorXd ret(params.size());
    for (int ito = 0; ito < size(); ++ito) {
        const ParameterRef& param = params[ito];
        int ifrom = subset().indexOf(param);
        if (ifrom == -1)
            ret[ito] = param.savedValue() / param.masterScale();
        else
            ret[ito] = _values[ifrom];
    }
    return ret;
}


void ValueSet::reset()
{
    _values = savedValues();
}

void ValueSet::apply() const
{
    for (int i = 0; i < size(); ++i) {
        ParameterRef param = subset()[i];
        param.savedValue() = _values[i] * _scales[i];
    }
}

void ValueSet::paste(const ValueSet& from)
{

    for(int ifrom = 0; ifrom < from.size(); ++ifrom){
        int ito = _subset->indexOf(from.subset()[ifrom]);
        if (ito != -1){
            _values[ito] = from._values[ifrom];
            _duals[ito] = from._duals[ifrom];
        }
    }
}

void ValueSet::paste(const HValueSet from)
{
    paste(*from);
}

void ValueSet::initFrom(const ValueSet& from)
{

    for(int ito = 0; ito < size(); ++ito){
        int ifrom = from.subset().indexOf((*_subset)[ito]);
        if (ifrom != -1){
            _values[ito] = from._values[ifrom];
            _duals[ito] = from._duals[ifrom];
        }
    }
}

bool ValueSet::setForDerivative(ParameterRef param)
{
    _duals.assign(size(),0.0);
    int i = subset().indexOf(param);
    if (i != -1){
        _duals[i] = 1.0;
        return true;
    } else {
        return false;
    }
}

void ValueSet::setForDerivative(const Eigen::VectorXd& dir)
{
    if (!(dir.size() == size())){
        std::stringstream ss;
        ss << "Dimension count of direction vector (" << dir.size() << ") "
           << "doesn't match the number of parameters in subset ("
           << size() <<")";
        throw Base::ValueError(ss.str());
    }
    for(int i = 0; i < size(); ++i)
        _duals[i] = dir[i];
}

void ValueSet::resetDerivatives()
{
    _duals.assign(size(), 0.0);
}

void ValueSet::setDual_solver(const ParameterRef& param, double val)
{
    _duals[subset().indexOf(param)] = val;
}

void ValueSet::setDual(const ParameterRef& param, double val)
{
    int i = subset().indexOf(param);
    _duals[i] = val / _scales[i];
}

void ValueSet::setReal(const ParameterRef& param, double val)
{
    if (_passthru){
        param.savedValue() = val;
    } else {
        int i = subset().has(param) ? subset().indexOf(param) : -1;
        if (i == -1){
            throw Py::KeyError("Paramater " + param.repr() + " is not in this value set.");
        } else {
            _values[i] = val / _scales[i];
        }
    }
}

void ValueSet::set(const ParameterRef& param, Base::DualNumber val)
{
    param.throwNull();
    setReal(param, val.re);
    if (! _passthru)
        setDual(param, val.du);
}

Base::DualNumber ValueSet::operator[](const ParameterRef& param) const
{
    int i = subset().indexOf(param);
    if (i != -1){
        return DualNumer(_values[i], _duals[i]) * _scales[i];
    } else {
        return DualNumer(param.savedValue(), 0.0);
    }
}

Base::DualNumber ValueSet::operator[](int index) const
{
    return DualNumer(_values[index], _duals[index]) * _scales[index];
}

Base::DualNumber ValueSet::get(const ParameterRef& param) const
{
    int i = subset().has(param) ? subset().indexOf(param) : -1;
    if (i != -1){
        return DualNumer(_values[i], _duals[i]) * _scales[i];
    } else {
        return DualNumer(param.savedValue(), 0.0);
    }
}

HValueSet ValueSet::self() const
{
    return HValueSet(_twin, false);
}

PyObject* ValueSet::getPyObject()
{
    return Py::new_reference_to(_twin);
}

void ValueSet::operator=(const ValueSet& other)
{
    checkSameSet(other);
    _values = other._values;
    _duals = other._duals;
}

void ValueSet::operator=(const HValueSet& other)
{
    operator=(*other);
}

void ValueSet::operator=(const Eigen::VectorXd& vals)
{
    checkSameSize(vals.size());
    _values = vals;
}

void ValueSet::operator+=(const ValueSet& other)
{
    checkSameSet(other);
    _values += other._values;
}

void ValueSet::operator*=(double mult)
{
    _values *= mult;
}

namespace FCS { // friends to be declared in the same namespace it removes the warnings too
    
HValueSet operator+(const HValueSet& a, const HValueSet& b)
{
    a->checkSameSet(*b);
    HValueSet hret = ValueSet::make(a->_subset, Eigen::VectorXd(0), true);
    hret->_values = a->_values + b->_values;
    return hret;
}

HValueSet operator+(const HValueSet &a, const Eigen::VectorXd &b)
{
    a->checkSameSize(b.size());
    HValueSet hret = ValueSet::make(a->_subset, Eigen::VectorXd(0), true);
    hret->_values = a->_values + b;
    return hret;
}

HValueSet operator+(const Eigen::VectorXd &a, const HValueSet &b)
{
    return b+a;
}

HValueSet operator-(const HValueSet &a, const HValueSet &b)
{
    a->checkSameSet(*b);
    HValueSet hret = ValueSet::make(a->_subset, Eigen::VectorXd(0), true);
    hret->_values = a->_values - b->_values;
    return hret;
}

HValueSet operator-(const HValueSet &a, const Eigen::VectorXd &b)
{
    a->checkSameSize(b.size());
    HValueSet hret = ValueSet::make(a->_subset, Eigen::VectorXd(0), true);
    hret->_values = a->_values - b;
    return hret;
}

HValueSet operator-(const Eigen::VectorXd &b, const HValueSet &a)
{
    a->checkSameSize(b.size());
    HValueSet hret = ValueSet::make(a->_subset, Eigen::VectorXd(0), true);
    hret->_values = b - a->_values;
    return hret;
}

HValueSet operator*(const HValueSet &a, double b)
{
    HValueSet hret = ValueSet::make(a->_subset, Eigen::VectorXd(0), true);
    hret->_values = a->_values * b;
    return hret;
}

HValueSet operator*(double b, const HValueSet &a)
{
    return a*b;
}

HValueSet operator/(const HValueSet &a, double b)
{
    HValueSet hret = ValueSet::make(a->_subset, Eigen::VectorXd(0), true);
    hret->_values = a->_values / b;
    return hret;
}

} //namespace FCS
