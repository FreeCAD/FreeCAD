#include "PreCompiled.h"

#include "ParaConic.h"
#include "G2D/ParaConicPy.h"
#include "PyUtils.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaConic, FCS::G2D::ParaCurve);

ParaConic::ParaConic()
{
    initAttrs();
}

ParaConic::ParaConic(HParaPoint p0, HParaPoint p1)
    : ParaConic()
{
    this->p0 = p0;
    this->p1 = p1;
}

void FCS::G2D::ParaConic::initAttrs()
{
    ParaCurve::initAttrs();

    tieAttr_Child(p0.upcast<ParaObject>(), "p0", &ParaPointPy::Type, true);
    tieAttr_Child(p1.upcast<ParaObject>(), "p1", &ParaPointPy::Type, true);
}

std::vector<ParameterRef> ParaConic::makeParameters(HParameterStore into)
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

Position ParaConic::value(const ValueSet& vals, DualNumber u)
{
    return p0->value(vals) * (1.0 - u) + p1->value(vals) * u;
}

Vector ParaConic::tangent(const ValueSet& vals, DualNumber u)
{
    (void)u;
    return p1->value(vals) - p0->value(vals);
}

Vector ParaConic::tangentAtXY(const ValueSet& vals, Position p)
{
    (void)p;
    return tangent(vals, 0.0);
}

DualNumber ParaConic::length(const ValueSet& vals, DualNumber u0, DualNumber u1)
{
    return length(vals) * (u1-u0);
}

DualNumber ParaConic::length(const ValueSet& vals)
{
    return (p1->value(vals) - p0->value(vals)).length();
}

DualNumber ParaConic::pointOnCurveErrFunc(const ValueSet& vals, Position p)
{
    return Vector::cross(tangent(vals, 0.0).normalized(), p - p0->value(vals));
}

PyObject* ParaConic::getPyObject()
{
    if (!_twin){
        _twin = new ParaConicPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}


