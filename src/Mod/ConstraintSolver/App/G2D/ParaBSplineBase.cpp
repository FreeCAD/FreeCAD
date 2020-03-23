#include "PreCompiled.h"

#include "ParaBSplineBase.h"
#include "G2D/ParaBSplineBasePy.h"
#include "PyUtils.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaBSplineBase, FCS::G2D::ParaCurve);

ParaBSplineBase::ParaBSplineBase()
{
    initAttrs();
}

ParaBSplineBase::ParaBSplineBase(HParaPoint p0, HParaPoint p1)
    : ParaBSplineBase()
{
    this->p0 = p0;
    this->p1 = p1;
}

void FCS::G2D::ParaBSplineBase::initAttrs()
{
    ParaCurve::initAttrs();

    tieAttr_Child(p0.upcast<ParaObject>(), "p0", &ParaPointPy::Type, true);
    tieAttr_Child(p1.upcast<ParaObject>(), "p1", &ParaPointPy::Type, true);
}

std::vector<ParameterRef> ParaBSplineBase::makeParameters(HParameterStore into)
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

Position ParaBSplineBase::value(const ValueSet& vals, DualNumber u)
{
    return p0->value(vals) * (1.0 - u) + p1->value(vals) * u;
}

Vector ParaBSplineBase::tangent(const ValueSet& vals, DualNumber u)
{
    (void)u;
    return p1->value(vals) - p0->value(vals);
}

Vector ParaBSplineBase::tangentAtXY(const ValueSet& vals, Position p)
{
    (void)p;
    return tangent(vals, 0.0);
}

DualNumber ParaBSplineBase::length(const ValueSet& vals, DualNumber u0, DualNumber u1)
{
    return length(vals) * (u1-u0);
}

DualNumber ParaBSplineBase::length(const ValueSet& vals)
{
    return (p1->value(vals) - p0->value(vals)).length();
}

DualNumber ParaBSplineBase::pointOnCurveErrFunc(const ValueSet& vals, Position p)
{
    return Vector::cross(tangent(vals, 0.0).normalized(), p - p0->value(vals));
}

PyObject* ParaBSplineBase::getPyObject()
{
    if (!_twin){
        _twin = new ParaBSplineBasePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}


