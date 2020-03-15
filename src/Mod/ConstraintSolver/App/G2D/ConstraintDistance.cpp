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
    this->p1 = new ParaShape<ParaPoint>(new ParaTransform(), p1);
    this->p2 = new ParaShape<ParaPoint>(new ParaTransform(), p2);
    this->dist = dist;
}

ConstraintDistance::ConstraintDistance(HParaPoint p1, HParaPoint p2, FCS::HParameterStore store)
    : ConstraintDistance()
{
    this->p1 = new ParaShape<ParaPoint>(new ParaTransform(), p1);
    this->p2 = new ParaShape<ParaPoint>(new ParaTransform(), p2);
    this->makeParameters(store);
}

void ConstraintDistance::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Parameter(dist, "dist", true, true, 1.0);

    tieAttr_Shape(reinterpret_cast<HParaObject &>(p1), "p1", ParaPoint::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(p2), "p2", ParaPoint::getClassTypeId());
}

void ConstraintDistance::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

Base::DualNumber ConstraintDistance::error1(const ValueSet& vals) const
{
    Position pp1 = p1->placement->value(vals) * p1->tshape()(vals);
    Position pp2 = p2->placement->value(vals) * p2->tshape()(vals);
    return vals[dist] - (pp1 - pp2).length();
}

std::vector<Base::DualNumber> ConstraintDistance::caluclateDatum(const ValueSet& vals)
{
    throwIfIncomplete();
    Position pp1 = p1->placement->value(vals) * p1->tshape()(vals);
    Position pp2 = p2->placement->value(vals) * p2->tshape()(vals);
    return { (pp1 - pp2).length() };
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
