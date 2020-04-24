#include "PreCompiled.h"

#include "ConstraintTangentEllipseLine.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintTangentEllipseLinePy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintTangentEllipseLine, FCS::SimpleConstraint);


ConstraintTangentEllipseLine::ConstraintTangentEllipseLine()
    : ellipse(Py::None()), line(Py::None())
{
    initAttrs();
}

void ConstraintTangentEllipseLine::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(ellipse, "ellipse", ParaEllipse::getClassTypeId());
    tieAttr_Shape(line, "line", ParaLine::getClassTypeId());
}

Base::DualNumber ConstraintTangentEllipseLine::error1(const ValueSet& vals) const
{
    Placement plm_l = line->placement->value(vals);
    Placement plm_el = ellipse->placement->value(vals);
    Position p0 = plm_l * line->tshape().p0->value(vals);
    Vector normal = plm_l * line->tshape().tangent(vals, 0.0).rotate90ccw().normalized();
    Position f1 = plm_el * ellipse->tshape().getFocus1(vals);
    Position f2 = plm_el * ellipse->tshape().getFocus2(vals);
    //reflect f2 against the line, the distance from reflection to f1 should be 2*a
    Position f2_refl = f2 + Vector::dot(p0 - f2, normal) * 2 * normal;
    return (f2_refl - f1).length() - 2 * ellipse->tshape().getRMaj(vals);
}

void ConstraintTangentEllipseLine::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintTangentEllipseLine::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintTangentEllipseLinePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
