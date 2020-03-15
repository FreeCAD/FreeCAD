#include "PreCompiled.h"

#include "ConstraintTangentLineLine.h"
#include "G2D/ConstraintTangentLineLinePy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintTangentLineLine, FCS::Constraint);


ConstraintTangentLineLine::ConstraintTangentLineLine()
    : line1(Py::None()), line2(Py::None())
{
    initAttrs();
}

ConstraintTangentLineLine::ConstraintTangentLineLine(HShape_Line line1, HShape_Line line2, std::string label)
    : ConstraintTangentLineLine()
{
    this->line1 = line1;
    this->line2 = line2;
    this->label = label;
}

void ConstraintTangentLineLine::initAttrs()
{
    Constraint::initAttrs();

    tieAttr_Shape(reinterpret_cast<HParaObject &>(line1), "line1", ParaLine::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(line2), "line2", ParaLine::getClassTypeId());
}

void ConstraintTangentLineLine::error(const ValueSet& vals, Base::DualNumber* returnbuf) const
{
    Placement plm1 = line1->placement->value(vals);
    Placement plm2 = line2->placement->value(vals);

    //angle between lines
    Vector tang1 = (plm1 * line1->tshape().tangent(vals, 0.0));
    Vector tang2 = (plm2 * line2->tshape().tangent(vals, 0.0));
    tang1 = tang1 * (line1->reversed ? -1 : 1);
    tang2 = tang2 * (line2->reversed ? -1 : 1);
    returnbuf[0] = signedAngle(atan2(tang2.y, tang2.x) - atan2(tang1.y, tang1.x) * _revers);

    //distance between lines (constrain area of polygon made of vertices to zero)
    Position p1 = plm1 * line1->tshape().p0->value(vals);
    Position p2 = plm1 * line1->tshape().p1->value(vals);
    Position p3 = plm2 * line2->tshape().p0->value(vals);
    Position p4 = plm2 * line2->tshape().p1->value(vals);
    DualNumber area = 0.5 * (
          p1.x * p2.y - p1.y * p2.x
        + p2.x * p3.y - p2.y * p3.x
        + p3.x * p4.y - p3.y * p4.x
        + p4.x * p1.y - p4.y * p1.x
    );
    //scale the area to be ~1 in magnitude
    returnbuf[1] = area / (tang1.sqlength() + tang2.sqlength());
}

PyObject* ConstraintTangentLineLine::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintTangentLineLinePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
