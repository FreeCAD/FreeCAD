#include "PreCompiled.h"

#include "ParaLine.h"
#include "G2D/ParaLinePy.h"
#include "PyUtils.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaLine, FCS::G2D::ParaCurve);

ParaLine::ParaLine()
    :p0(Py::None()), p1(Py::None())
{
    initAttrs();
}

ParaLine::ParaLine(HParaPoint p0, HParaPoint p1)
    : ParaLine()
{
    this->p0 = p0;
    this->p1 = p1;
}

void FCS::G2D::ParaLine::initAttrs()
{
    ParaCurve::initAttrs();

    tieAttr_Child(p0, "p0", &ParaPointPy::Type, true);
    tieAttr_Child(p1, "p1", &ParaPointPy::Type, true);
}

std::vector<ParameterRef> ParaLine::makeParameters(HParameterStore into)
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

Position ParaLine::value(const ValueSet& vals, DualNumber u)
{
    return p0->value(vals) * (1.0 - u) + p1->value(vals) * u;
}

Vector ParaLine::tangent(const ValueSet& vals, DualNumber u)
{
    return p1->value(vals) - p0->value(vals);
}

Vector ParaLine::tangentAtXY(const ValueSet& vals, Position p)
{
    return tangent(vals, 0.0);
}

DualNumber ParaLine::length(const ValueSet& vals, DualNumber u0, DualNumber u1)
{
    return length(vals) * (u1-u0);
}

DualNumber ParaLine::length(const ValueSet& vals)
{
    return (p1->value(vals) - p0->value(vals)).length();
}

DualNumber ParaLine::pointOnCurveErrFunc(const ValueSet& vals, Position p)
{
    return Vector::cross(tangent(vals, 0.0).normalized(), p - p0->value(vals));
}

PyObject* ParaLine::getPyObject()
{
    if (!_twin){
        _twin = new ParaLinePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}


