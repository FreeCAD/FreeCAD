#include "PreCompiled.h"

#include "ParaVector.h"
#include "G2D/ParaVectorPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaVector, FCS::ParaObject);

Vector ParaVector::operator()(const ValueSet &vals) const
{
    return Vector(vals[x], vals[y]);
}

ParaVector::ParaVector()
{
    initAttrs();
}

ParaVector::ParaVector(ParameterRef x, ParameterRef y)
    : ParaVector()
{
    this->x = x;
    this->y = y;
}

void FCS::G2D::ParaVector::initAttrs()
{
    _attrs = {
        {&x, "x", true, 0.0},
        {&y, "y", true, 0.0},
    };
}

PyObject* ParaVector::getPyObject()
{
    if (!_twin){
        new ParaVectorPy(this);
        assert(_twin);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }

}
