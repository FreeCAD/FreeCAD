#include "PreCompiled.h"

#include "ConstraintPointCoincident.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintPointCoincidentPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintPointCoincident, FCS::Constraint);


ConstraintPointCoincident::ConstraintPointCoincident()
    : p1(Py::None()), p2(Py::None())
{
    initAttrs();
}

ConstraintPointCoincident::ConstraintPointCoincident(HShape_Point p1, HShape_Point p2, std::string label)
    : ConstraintPointCoincident()
{
    this->p1 = p1;
    this->p2 = p2;
    this->label = label;
}

void ConstraintPointCoincident::initAttrs()
{
    Constraint::initAttrs();

    tieAttr_Shape(p1.upcast<ParaObject>(), "p1", ParaPoint::getClassTypeId());
    tieAttr_Shape(p2.upcast<ParaObject>(), "p2", ParaPoint::getClassTypeId());
}

void ConstraintPointCoincident::error(const ValueSet& vals, Base::DualNumber* returnbuf) const
{
    Position p1v = p1->placement->value(vals) * p1->tshape()(vals);
    Position p2v = p2->placement->value(vals) * p2->tshape()(vals);
    Vector diff = p1v - p2v;
    returnbuf[0] = diff.x;
    returnbuf[1] = diff.y;
}

void ConstraintPointCoincident::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintPointCoincident::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintPointCoincidentPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
