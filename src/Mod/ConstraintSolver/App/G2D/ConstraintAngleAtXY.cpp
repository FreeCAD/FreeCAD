#include "PreCompiled.h"

#include "ConstraintAngleAtXY.h"
#include "G2D/ConstraintAngleAtXYPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintAngleAtXY, FCS::G2D::ConstraintAngle);


ConstraintAngleAtXY::ConstraintAngleAtXY()
    : p(nullptr), crv1(nullptr), crv2(nullptr)
{
    initAttrs();
}

void ConstraintAngleAtXY::initAttrs()
{
    ConstraintAngle::initAttrs();

    tieAttr_Shape(p.upcast<ParaObject>(), "p", ParaPoint::getClassTypeId());
    tieAttr_Shape(crv1.upcast<ParaObject>(), "crv1", ParaCurve::getClassTypeId());
    tieAttr_Shape(crv2.upcast<ParaObject>(), "crv2", ParaCurve::getClassTypeId());
}

PyObject* ConstraintAngleAtXY::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintAngleAtXYPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

Base::DualNumber ConstraintAngleAtXY::calculateAngle(const ValueSet& vals) const
{
    Position pos = p->placement->value(vals) * p->tshape().value(vals);
    Placement plm1 = crv1->placement->value(vals);
    Placement plm2 = crv2->placement->value(vals);
    Vector tang1 = (plm1 * crv1->tshape().tangentAtXY(vals, plm1.inverse() * pos));
    Vector tang2 = (plm2 * crv2->tshape().tangentAtXY(vals, plm2.inverse() * pos));
    tang1 = tang1 * (crv1->reversed ? -1 : 1);
    tang2 = tang2 * (crv2->reversed ? -1 : 1);
    return atan2(tang2.y, tang2.x) - atan2(tang1.y, tang1.x);
}
