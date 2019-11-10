#include "PreCompiled.h"

#include "ValueSet.h"
#include "ValueSetPy.h"

using namespace GCS;
using DualNumer = Base::DualNumber;

GCS::ValueSet::ValueSet(GCS::HParameterSubset subset)
    : _subset(subset)
{
    reset();
}

HValueSet ValueSet::make(HParameterSubset subset)
{
    ValueSet* obj = new ValueSet(subset);
    PyObject* pyobj = new ValueSetPy(obj);
    obj->_twin = pyobj;
    return HValueSet(pyobj, /*new_reference=*/true);
}

HValueSet ValueSet::makeTrivial(HParameterStore store)
{
    HParameterSubset s = ParameterSubset::make(store);
    return make(s);
}

HValueSet ValueSet::copy() const
{
    HValueSet cpy = make(this->_subset);
    cpy->_values = this->_values;
    cpy->_duals = this->_duals;
    cpy->_scales = this->_scales;
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

void ValueSet::reset()
{
    _values.resize(size());
    _duals.resize(size(), 0.0);
    _scales.resize(size(), 0.0);
    for (int i = 0; i < size(); ++i) {
        _scales[i] = subset()[i].masterScale();
    }
    _values = savedValues();
}

void ValueSet::apply() const
{
    for (int i = 0; i < size(); ++i) {
        ParameterRef param = subset()[i];
        param.savedValue() = _values[i] * _scales[i];
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

Base::DualNumber ValueSet::operator[](ParameterRef param) const
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

Base::DualNumber ValueSet::get(ParameterRef param) const
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
