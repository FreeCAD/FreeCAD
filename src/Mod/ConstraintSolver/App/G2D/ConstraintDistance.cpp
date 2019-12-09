#include "PreCompiled.h"

#include "ConstraintDistance.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintDistancePy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintDistance, FCS::SimpleConstraint);


ConstraintDistance::ConstraintDistance()
    :p1(Py::None()), p2(Py::None())
{
    initAttrs();
}

ConstraintDistance::ConstraintDistance(HParaPoint p1, HParaPoint p2, FCS::ParameterRef dist)
    :ConstraintDistance()
{
    this->p1 = p1;
    this->p2 = p2;
    this->dist = dist;
}

ConstraintDistance::ConstraintDistance(HParaPoint p1, HParaPoint p2, FCS::HParameterStore store)
    : ConstraintDistance()
{
    this->p1 = p1;
    this->p2 = p2;
    this->makeParameters(store);
}

void ConstraintDistance::initAttrs()
{
    _attrs = {
        {&dist, "dist", true, 1.0},
    };
    _children = {
        {&p1, "p1", &ParaPointPy::Type},
        {&p2, "p2", &ParaPointPy::Type},
    };
}

void ConstraintDistance::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

Base::DualNumber ConstraintDistance::error1(const ValueSet& vals) const
{
    return vals[dist] - (p1->pos(vals) - p2->pos(vals)).length();
}

PyObject* ConstraintDistance::getPyObject()
{
    if (!_twin){
        new ConstraintDistancePy(this);
        assert(_twin);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
