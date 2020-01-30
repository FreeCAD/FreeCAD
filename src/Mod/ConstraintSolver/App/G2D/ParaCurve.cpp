#include "PreCompiled.h"

#include "ParaCurve.h"
#include "G2D/ParaCurvePy.h"

#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::G2D::ParaCurve, FCS::G2D::ParaGeometry2D);


ParaCurve::ParaCurve()
    : p0(Py::None()), p1(Py::None())
{

}

void FCS::G2D::ParaCurve::initAttrs()
{
    ParaGeometry2D::initAttrs();

    tieAttr_Parameter(u0, "u0", !isFull(), 0.0);
    tieAttr_Parameter(u1, "u1", !isFull(), 0.0);
    tieAttr_Child(p0, "p0", &ParaPointPy::Type, !isFull());
    tieAttr_Child(p1, "p1", &ParaPointPy::Type, !isFull());
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

Vector ParaCurve::tangentAtXY(const ValueSet& /*vals*/, Position /*p*/)
{
    throwFunctionNotSupported("tangentAtXY");
}

Vector ParaCurve::D(const ValueSet& /*vals*/, DualNumber /*u*/, int /*n*/)
{
    throwFunctionNotSupported("D");
}

DualNumber ParaCurve::length(const ValueSet& /*vals*/, DualNumber /*u0*/, DualNumber /*u1*/)
{
    throwFunctionNotSupported("length");
}

DualNumber ParaCurve::length(const ValueSet& vals)
{
    if (! u0.isNull() && ! u1.isNull())
        return length(vals, vals[u0], vals[u1]);
    else
        return fullLength(vals);
}

DualNumber ParaCurve::fullLength(const ValueSet& /*vals*/)
{
    throwFunctionNotSupported("fullLength");
}

DualNumber ParaCurve::pointOnCurveErrFunc(const ValueSet& /*vals*/, Position /*p*/)
{
    throwFunctionNotSupported("pointOnCurveErrFunc");
}

void ParaCurve::throwFunctionNotSupported(std::string funcname) const
{
    throw Base::NotImplementedError(funcname + " is not implemented for curve type " + getTypeId().getName());
}
