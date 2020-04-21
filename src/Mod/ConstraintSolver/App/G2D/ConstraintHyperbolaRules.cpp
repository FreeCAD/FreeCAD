#include "PreCompiled.h"

#include "ConstraintHyperbolaRules.h"
//#include "G2D/ConstraintHyperbolaRulesPy.h"

#include "G2D/ParaHyperbolaPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintHyperbolaRules, FCS::Constraint);


ConstraintHyperbolaRules::ConstraintHyperbolaRules()
    : hy(Py::None())
{
    initAttrs();
}

ConstraintHyperbolaRules::ConstraintHyperbolaRules(HParaHyperbola hy)
    : ConstraintHyperbolaRules()
{
    this->hy = hy;
}

void ConstraintHyperbolaRules::initAttrs()
{
    Constraint::initAttrs();

    tieAttr_Child(hy, "hy", &ParaHyperbolaPy::Type);
}

void ConstraintHyperbolaRules::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

int ConstraintHyperbolaRules::rank() const
{
    int ret = 0;
    if (!hy->focus1.isNone())
        ret += 2;
    if (!hy->focus2.isNone())
        ret += 2;
    if (!hy->majorDiameterLine.isNone())
        ret += 4;
    if (!hy->minorDiameterLine.isNone())
        ret += 4;
    return ret;
}

void ConstraintHyperbolaRules::error(const ValueSet& vals, Base::DualNumber* returnbuf) const
{
    int i = 0;

    if (! hy->focus1.isNone()){
        Position f1 = hy->getFocus1(vals);
        Position p = hy->focus1->value(vals);
        returnbuf[i++] = f1.x - p.x;
        returnbuf[i++] = f1.y - p.y;
    }

    if (! hy->focus2.isNone()){
        Position f2 = hy->getFocus2(vals);
        Position p = hy->focus2->value(vals);
        returnbuf[i++] = f2.x - p.x;
        returnbuf[i++] = f2.y - p.y;
    }

    Position c = hy->center->value(vals);
    Position a = hy->majorAxisPoint->value(vals);

    if (! hy->majorDiameterLine.isNone()){
        Position p0t = 2 * c + -a;
        Position p1t = a;
        Position p0 = hy->majorDiameterLine->p0->value(vals);
        Position p1 = hy->majorDiameterLine->p1->value(vals);
        returnbuf[i++] = p0t.x - p0.x;
        returnbuf[i++] = p0t.y - p0.y;
        returnbuf[i++] = p1t.x - p1.x;
        returnbuf[i++] = p1t.y - p1.y;
    }

    if (! hy->minorDiameterLine.isNone()){
        Vector yax = (a - c).normalized().rotate90ccw();
        DualNumber b = hy->getRMin(vals);
        Position p0t = a - yax * b;
        Position p1t = a + yax * b;
        Position p0 = hy->minorDiameterLine->p0->value(vals);
        Position p1 = hy->minorDiameterLine->p1->value(vals);
        returnbuf[i++] = p0t.x - p0.x;
        returnbuf[i++] = p0t.y - p0.y;
        returnbuf[i++] = p1t.x - p1.x;
        returnbuf[i++] = p1t.y - p1.y;
    }
}

//PyObject* ConstraintHyperbolaRules::getPyObject()
//{
//    if (!_twin){
//        _twin = new ConstraintHyperbolaRulesPy(this);
//        return _twin;
//    } else  {
//        return Py::new_reference_to(_twin);
//    }
//}
