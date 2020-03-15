#include "PreCompiled.h"

#include "ConstraintTangentCircleLine.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintTangentCircleLinePy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>
#include <src/Mod/ConstraintSolver/App/G2D/ParaPlacementPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintTangentCircleLine, FCS::SimpleConstraint);


ConstraintTangentCircleLine::ConstraintTangentCircleLine()
    : circle(Py::None()), line(Py::None())
{
    initAttrs();
}

void ConstraintTangentCircleLine::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(reinterpret_cast<HParaObject &>(circle), "circle", ParaCircle::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(line), "line", ParaLine::getClassTypeId());
}

Base::DualNumber ConstraintTangentCircleLine::error1(const ValueSet& vals) const
{
    Placement plm_l = line->placement->value(vals);
    Position p0 = plm_l * line->tshape().p0->value(vals);
    Vector dir = plm_l * line->tshape().tangent(vals, 0.0).normalized() * (line->reversed ? -1.0 : 1.0);
    Vector distdir = dir.rotate90ccw();
    Position pointpos = circle->placement->value(vals) * circle->tshape().center->value(vals);
    return Vector::dot((pointpos - p0), distdir) - vals[circle->tshape().radius] * (circle->reversed ? -1.0 : 1.0);
}

void ConstraintTangentCircleLine::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintTangentCircleLine::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintTangentCircleLinePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
