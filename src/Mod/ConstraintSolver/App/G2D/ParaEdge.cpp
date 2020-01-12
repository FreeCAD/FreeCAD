#include "PreCompiled.h"

#include "ParaEdge.h"
#include "G2D/ParaEdgePy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::G2D::ParaEdge, FCS::G2D::ParaGeometry2D);


void FCS::G2D::ParaEdge::initAttrs()
{
    _attrs = {
        {&u0, "u0", true, 0.0},
        {&u1, "u1", true, 0.0},
    };
}

PyObject* ParaEdge::getPyObject()
{
    if (!_twin){
        _twin = new ParaEdgePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

Vector ParaEdge::tangentAtXY(const ValueSet& /*vals*/, Point /*p*/)
{
    throw Base::NotImplementedError("tangentAtXY is not implemented for this edge type");
}
