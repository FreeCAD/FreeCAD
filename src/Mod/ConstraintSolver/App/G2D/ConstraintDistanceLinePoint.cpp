#include "PreCompiled.h"

#include "ConstraintDistanceLinePoint.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintDistanceLinePointPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>
#include <src/Mod/ConstraintSolver/App/G2D/ParaPlacementPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintDistanceLinePoint, FCS::SimpleConstraint);


ConstraintDistanceLinePoint::ConstraintDistanceLinePoint()
    : line(Py::None()), point(Py::None())
{
    initAttrs();
}

void ConstraintDistanceLinePoint::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(reinterpret_cast<HParaObject &>(line), "line", ParaLine::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(point), "point", ParaPoint::getClassTypeId());
    tieAttr_Parameter(dist, "dist", true, true, 1.0);
}

Base::DualNumber ConstraintDistanceLinePoint::error1(const ValueSet& vals) const
{
    DualNumber truedist = calculateDistance(vals);
    return truedist - vals[dist] * _revers;
}

std::vector<Base::DualNumber> ConstraintDistanceLinePoint::caluclateDatum(const ValueSet& vals)
{
    throwIfIncomplete();
    return {calculateDistance(vals) * _revers};
}

void ConstraintDistanceLinePoint::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintDistanceLinePoint::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintDistanceLinePointPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

DualNumber ConstraintDistanceLinePoint::calculateDistance(const ValueSet& vals) const
{
    Placement plm_l = line->placement->value(vals);
    Position p0 = plm_l * line->tshape().p0->value(vals);
    Vector dir = plm_l * line->tshape().tangent(vals, 0.0).normalized() * (line->reversed ? -1.0 : 1.0);
    Vector distdir = dir.rotate90cw();
    Position pointpos = point->placement->value(vals) * point->tshape().value(vals);
    return Vector::dot((pointpos - p0), distdir);
}
