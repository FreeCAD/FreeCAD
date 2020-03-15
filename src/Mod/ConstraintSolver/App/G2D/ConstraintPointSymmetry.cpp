#include "PreCompiled.h"

#include "ConstraintPointSymmetry.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintPointSymmetryPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintPointSymmetry, FCS::Constraint);


ConstraintPointSymmetry::ConstraintPointSymmetry()
    : p1(Py::None()), p2(Py::None()), pc(Py::None())
{
    initAttrs();
}

ConstraintPointSymmetry::ConstraintPointSymmetry(HShape_Point p1, HShape_Point p2, HShape_Point pc, std::string label)
    : ConstraintPointSymmetry()
{
    this->p1 = p1;
    this->p2 = p2;
    this->pc = pc;
    this->label = label;
}

void ConstraintPointSymmetry::initAttrs()
{
    Constraint::initAttrs();

    tieAttr_Shape(reinterpret_cast<HParaObject &>(p1), "p1", ParaPoint::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(p2), "p2", ParaPoint::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(pc), "pc", ParaPoint::getClassTypeId());
}

void ConstraintPointSymmetry::error(const ValueSet& vals, Base::DualNumber* returnbuf) const
{
    Position p1v = p1->placement->value(vals) * p1->tshape()(vals);
    Position p2v = p2->placement->value(vals) * p2->tshape()(vals);
    Position pcv = pc->placement->value(vals) * pc->tshape()(vals);
    Vector diff = (p1v + p2v) * 0.5 - pcv;
    returnbuf[0] = diff.x;
    returnbuf[1] = diff.y;
}

void ConstraintPointSymmetry::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintPointSymmetry::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintPointSymmetryPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
