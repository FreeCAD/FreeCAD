#include "PreCompiled.h"

#include "ConstraintEllipseRules.h"
//#include "G2D/ConstraintEllipseRulesPy.h"

#include "G2D/ParaEllipsePy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintEllipseRules, FCS::Constraint);


ConstraintEllipseRules::ConstraintEllipseRules()
    : e(Py::None())
{
    initAttrs();
}

ConstraintEllipseRules::ConstraintEllipseRules(HParaEllipse e)
    : ConstraintEllipseRules()
{
    this->e = e;
}

void ConstraintEllipseRules::initAttrs()
{
    Constraint::initAttrs();

    tieAttr_Child(e, "e", &ParaEllipsePy::Type);
}

void ConstraintEllipseRules::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

int ConstraintEllipseRules::rank() const
{
    int ret = 0;
    if (!e->focus2.isNone())
        ret += 2;
    if (!e->majorDiameterLine.isNone())
        ret += 4;
    if (!e->minorDiameterLine.isNone())
        ret += 4;
    return ret;
}

void ConstraintEllipseRules::error(const ValueSet& vals, Base::DualNumber* returnbuf) const
{
    int i = 0;

    if (! e->focus2.isNone()){
        Position f2 = e->getFocus2(vals);
        Position p = e->focus2->value(vals);
        returnbuf[i++] = f2.x - p.x;
        returnbuf[i++] = f2.y - p.y;
    }

    if (! e->majorDiameterLine.isNone()){
        Position p0t = e->value(vals, 0.5*TURN);
        Position p1t = e->value(vals, 0.0);
        Position p0 = e->majorDiameterLine->p0->value(vals);
        Position p1 = e->majorDiameterLine->p1->value(vals);
        returnbuf[i++] = p0t.x - p0.x;
        returnbuf[i++] = p0t.y - p0.y;
        returnbuf[i++] = p1t.x - p1.x;
        returnbuf[i++] = p1t.y - p1.y;
    }

    if (! e->minorDiameterLine.isNone()){
        Position p0t = e->value(vals, -0.25*TURN);
        Position p1t = e->value(vals, 0.25*TURN);
        Position p0 = e->minorDiameterLine->p0->value(vals);
        Position p1 = e->minorDiameterLine->p1->value(vals);
        returnbuf[i++] = p0t.x - p0.x;
        returnbuf[i++] = p0t.y - p0.y;
        returnbuf[i++] = p1t.x - p1.x;
        returnbuf[i++] = p1t.y - p1.y;
    }
}

//PyObject* ConstraintEllipseRules::getPyObject()
//{
//    if (!_twin){
//        _twin = new ConstraintEllipseRulesPy(this);
//        return _twin;
//    } else  {
//        return Py::new_reference_to(_twin);
//    }
//}
