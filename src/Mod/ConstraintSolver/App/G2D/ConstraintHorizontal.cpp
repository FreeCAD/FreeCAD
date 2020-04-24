#include "PreCompiled.h"

#include "ConstraintHorizontal.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintHorizontalPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintHorizontal, FCS::SimpleConstraint);


ConstraintHorizontal::ConstraintHorizontal()
    : p1(Py::None()), p2(Py::None())
{
    initAttrs();
}

void ConstraintHorizontal::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(p1, "p1", ParaPoint::getClassTypeId());
    tieAttr_Shape(p2, "p2", ParaPoint::getClassTypeId());
}

Base::DualNumber ConstraintHorizontal::error1(const ValueSet& vals) const
{
    Position p1v = p1->placement->value(vals) * p1->tshape()(vals);
    Position p2v = p2->placement->value(vals) * p2->tshape()(vals);
    Vector diff = p1v - p2v;
    return diff.y;
}

void ConstraintHorizontal::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintHorizontal::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintHorizontalPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
