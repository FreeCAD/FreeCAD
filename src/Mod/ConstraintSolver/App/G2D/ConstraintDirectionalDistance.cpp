#include "PreCompiled.h"

#include "ConstraintDirectionalDistance.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintDirectionalDistancePy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>
#include <src/Mod/ConstraintSolver/App/G2D/ParaPlacementPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintDirectionalDistance, FCS::SimpleConstraint);


ConstraintDirectionalDistance::ConstraintDirectionalDistance()
    : p1(Py::None()), p2(Py::None())
{
    initAttrs();
}

ConstraintDirectionalDistance::ConstraintDirectionalDistance(HShape_Point p1, HShape_Point p2, Vector direction)
    : ConstraintDirectionalDistance()
{
    setDirection(direction);
    this->p1 = p1;
    this->p2 = p2;
}

HConstraintDirectionalDistance ConstraintDirectionalDistance::makeConstraintHorizontalDistance(HShape_Point p1, HShape_Point p2)
{
    return new ConstraintDirectionalDistance(p1,p2,Vector(1.0, 0.0));
}

HConstraintDirectionalDistance ConstraintDirectionalDistance::makeConstraintVerticalDistance(HShape_Point p1, HShape_Point p2)
{
    return new ConstraintDirectionalDistance(p1,p2,Vector(0.0, 1.0));
}

void ConstraintDirectionalDistance::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(reinterpret_cast<HParaObject &>(p1), "p1", ParaPoint::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(p2), "p2", ParaPoint::getClassTypeId());
    tieAttr_Parameter(dist, "dist", true, true, 1.0);
}

Base::DualNumber ConstraintDirectionalDistance::error1(const ValueSet& vals) const
{
    Position p1v = p1->placement->value(vals) * p1->tshape()(vals);
    Position p2v = p2->placement->value(vals) * p2->tshape()(vals);
    return Vector::dot(p2v-p1v, _direction) * _revers - vals[dist];
}

std::vector<Base::DualNumber> ConstraintDirectionalDistance::caluclateDatum(const ValueSet& vals)
{
    throwIfIncomplete();
    Position p1v = p1->placement->value(vals) * p1->tshape()(vals);
    Position p2v = p2->placement->value(vals) * p2->tshape()(vals);
    return { Vector::dot(p2v-p1v, _direction) * _revers };
}

void ConstraintDirectionalDistance::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

void ConstraintDirectionalDistance::throwIfIncomplete() const
{
    SimpleConstraint::throwIfIncomplete();
    if (direction().length().re == 0.0)
        throw Py::ValueError(repr() + " has Direction not set");
}

PyObject* ConstraintDirectionalDistance::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintDirectionalDistancePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

HParaObject ConstraintDirectionalDistance::copy() const
{
    HConstraintDirectionalDistance cpy = SimpleConstraint::copy().downcast<ConstraintDirectionalDistance>();
    cpy->_direction = this->_direction;
    return cpy;
}

void ConstraintDirectionalDistance::setDirection(Vector newdirection)
{
    _direction = newdirection.normalized();
    //null out duals, they must be zero because direction never depends on any parameter
    this->_direction.x.du = 0; this->_direction.y.du = 0;
}
