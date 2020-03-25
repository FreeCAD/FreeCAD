#include "PreCompiled.h"

#include "ConstraintLength.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintLengthPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>
#include <src/Mod/ConstraintSolver/App/G2D/ParaPlacementPy.h>

#include <unordered_set>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintLength, FCS::SimpleConstraint);


ConstraintLength::ConstraintLength()
{
    initAttrs();
}

ConstraintLength::ConstraintLength(std::vector<HParaCurve> edges, ParameterRef length)
    : ConstraintLength()
{
    setEdges(edges);
    this->length = length;
}

void ConstraintLength::setEdges(const std::vector<HParaCurve> edges)
{
    _edges = edges;
    touch();
}

void ConstraintLength::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Parameter(length, "length", true, true, 1.0);
}

HParaObject ConstraintLength::copy() const
{
    HConstraintLength cpy = SimpleConstraint::copy().downcast<ConstraintLength>();
    cpy->_edges = this->_edges;
    return cpy;

}

void ConstraintLength::update()
{
    throwIfIncomplete();
    _parameters.clear();
    std::unordered_set<int> added;

    auto add = [&](const ParameterRef& v){
        v.throwNull();
        if (added.find(v.masterIndex()) != added.end())
            return;
        _parameters.push_back(v);
        added.insert(v.masterIndex());
    };
    for(auto& v : this->_attrs){
        if (!v.required && v.value->isNull())
            continue;
        add(*(v.value));
    };
    for(HParaCurve& crv : _edges){
        if (crv->isTouched())
            crv->update();
        for(const ParameterRef& r : crv->parameters()){
            add(r);
        };
    };

    _touched = false;
}

void ConstraintLength::throwIfIncomplete() const
{
    SimpleConstraint::throwIfIncomplete();
    if (_edges.size() == 0)
        throw Py::Exception(PyExc_LookupError, repr() + " has no edges assigned");
    int i = 0;
    for (const HParaCurve& crv : _edges){
        if (crv.isNone())
            throw Py::Exception(PyExc_LookupError,"Edge " + std::to_string(i) + " of " + repr() + " is None");
        crv->throwIfIncomplete();
        ++i;
    }
}

void ConstraintLength::setWeight(double weight)
{
    SimpleConstraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

DualNumber ConstraintLength::calculateLength(const ValueSet& vals) const
{
    DualNumber cum = 0;
    for (const HParaCurve& crv : _edges){
        cum = cum + crv->length(vals);
    }
    return cum;
}

Base::DualNumber ConstraintLength::error1(const ValueSet& vals) const
{
    return calculateLength(vals) - vals[length];
}

std::vector<Base::DualNumber> ConstraintLength::caluclateDatum(const ValueSet& vals)
{
    throwIfIncomplete();
    return {calculateLength(vals)};
}

PyObject* ConstraintLength::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintLengthPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
