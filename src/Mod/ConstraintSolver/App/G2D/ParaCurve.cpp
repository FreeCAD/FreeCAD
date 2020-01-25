#include "PreCompiled.h"

#include "ParaCurve.h"
#include "G2D/ParaCurvePy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::G2D::ParaCurve, FCS::G2D::ParaGeometry2D);


void FCS::G2D::ParaCurve::initAttrs()
{
    _attrs = {
        {&u0, "u0", true, 0.0},
        {&u1, "u1", true, 0.0},
    };
}

PyObject* ParaCurve::getPyObject()
{
    if (!_twin){
        _twin = new ParaCurvePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

Vector ParaCurve::tangentAtXY(const ValueSet& /*vals*/, Point /*p*/)
{
    throw Base::NotImplementedError("tangentAtXY is not implemented for this edge type");
}

Vector ParaCurve::D(const ValueSet& /*vals*/, DualNumber /*u*/, int /*n*/)
{
    throw Base::NotImplementedError("D is not implemented for this edge type");
}

DualNumber ParaCurve::length(const ValueSet& vals, DualNumber u0, DualNumber u1)
{
    throw Base::NotImplementedError("length is not implemented for this edge type");
}

DualNumber ParaCurve::length(const ValueSet& vals)
{
    if (! u0.isNull() && ! u1.isNull())
        return length(vals, vals[u0], vals[u1]);
    else
        return fullLength(vals);
}

DualNumber ParaCurve::fullLength(const ValueSet& vals)
{
    throw Base::NotImplementedError("fullLength is not implemented for this edge type");
}
