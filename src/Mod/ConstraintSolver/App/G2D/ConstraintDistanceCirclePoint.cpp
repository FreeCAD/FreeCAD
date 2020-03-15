#include "PreCompiled.h"

#include "ConstraintDistanceCirclePoint.h"
#include "G2D/ConstraintDistanceCirclePointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintDistanceCirclePoint, FCS::SimpleConstraint);


ConstraintDistanceCirclePoint::ConstraintDistanceCirclePoint()
    : circle(Py::None()), point(Py::None())
{
    initAttrs();
}

void ConstraintDistanceCirclePoint::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(reinterpret_cast<HParaObject &>(circle), "circle", ParaCircle::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(point), "point", ParaPoint::getClassTypeId());
    tieAttr_Parameter(dist, "dist", true, true, 1.0);
}

Base::DualNumber ConstraintDistanceCirclePoint::error1(const ValueSet& vals) const
{
    Placement plmc = circle->placement->value(vals);
    Placement plmp = point->placement->value(vals);
    Position circleCenter = plmc * circle->tshape().center->value(vals);
    Position pnt = plmp * point->tshape().value(vals);
    DualNumber dist_pc = (pnt - circleCenter).length();
    return vals[circle->tshape().radius] + vals[dist] * _revers - dist_pc;
}

std::vector<Base::DualNumber> ConstraintDistanceCirclePoint::caluclateDatum(const ValueSet& vals)
{
    throwIfIncomplete();
    Placement plmc = circle->placement->value(vals);
    Placement plmp = point->placement->value(vals);
    Position circleCenter = plmc * circle->tshape().center->value(vals);
    Position pnt = plmp * point->tshape().value(vals);
    DualNumber dist_pc = (pnt - circleCenter).length();
    DualNumber datum = (dist_pc - vals[circle->tshape().radius]) * _revers;
    return {datum};
}

void ConstraintDistanceCirclePoint::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintDistanceCirclePoint::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintDistanceCirclePointPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
