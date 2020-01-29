#include "PreCompiled.h"

#include "ParaCircle.h"
#include "G2D/ParaCirclePy.h"
#include "PyUtils.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaCircle, FCS::G2D::ParaCurve);

ParaCircle::ParaCircle()
{
    initAttrs();
}

ParaCircle::ParaCircle(HParaPoint p0, HParaPoint p1)
    : ParaCircle()
{
    this->p0 = p0;
    this->p1 = p1;
}

void FCS::G2D::ParaCircle::initAttrs()
{
    ParaCurve::initAttrs();

    tieAttr_Child(p0, "p0", &ParaPointPy::Type, true);
    tieAttr_Child(p1, "p1", &ParaPointPy::Type, true);
}

std::vector<ParameterRef> ParaCircle::makeParameters(HParameterStore into)
{
    bool init_u0 = u0.isNull();
    bool init_u1 = u1.isNull();
    std::vector<ParameterRef> ret = ParaCurve::makeParameters(into);
    if (init_u0){
        u0.savedValue() = 0.0;
        u0.fix();
    }
    if (init_u1){
        u1.savedValue() = 1.0;
        u1.fix();
    }
    return ret;
}

Position ParaCircle::value(const ValueSet& vals, DualNumber u)
{
    return p0->value(vals) * (1.0 - u) + p1->value(vals) * u;
}

Vector ParaCircle::tangent(const ValueSet& vals, DualNumber u)
{
    return p1->value(vals) - p0->value(vals);
}

Vector ParaCircle::tangentAtXY(const ValueSet& vals, Position p)
{
    return tangent(vals, 0.0);
}

DualNumber ParaCircle::length(const ValueSet& vals, DualNumber u0, DualNumber u1)
{
    return length(vals) * (u1-u0);
}

DualNumber ParaCircle::length(const ValueSet& vals)
{
    return (p1->value(vals) - p0->value(vals)).length();
}

DualNumber ParaCircle::pointOnCurveErrFunc(const ValueSet& vals, Position p)
{
    return Vector::cross(tangent(vals, 0.0).normalized(), p - p0->value(vals));
}

PyObject* ParaCircle::getPyObject()
{
    if (!_twin){
        _twin = new ParaCirclePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}


