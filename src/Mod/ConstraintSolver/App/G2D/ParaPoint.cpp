#include "PreCompiled.h"

#include "ParaPoint.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaPoint, FCS::ParaGeometry);

Vector ParaPoint::pos(const ValueSet& vals) const
{
    return Vector(vals[x], vals[y]);
}

ParaPoint::ParaPoint()
{
    initAttrs();
}

ParaPoint::ParaPoint(ParameterRef x, ParameterRef y)
    : ParaPoint()
{
    this->x = x;
    this->y = y;
}

void FCS::G2D::ParaPoint::initAttrs()
{
    _attrs = {
        {&x, "x", 0.0},
        {&y, "y", 0.0},
    };
}

PyObject* ParaPoint::getPyObject()
{
    if (!_twin){
        new ParaPointPy(this);
        assert(_twin);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }

}
