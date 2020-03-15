#include "PreCompiled.h"

#include "ParaGeometry2D.h"
#include "G2D/ParaGeometry2DPy.h"
#include "ParaShape.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::G2D::ParaGeometry2D, FCS::ParaGeometry);


PyObject* ParaGeometry2D::getPyObject()
{
    if (!_twin){
        _twin = new ParaGeometry2DPy(this);
        return _twin;
    } else {
        return Py::new_reference_to(_twin);
    }
}

HParaObject FCS::G2D::ParaGeometry2D::toShape()
{
    return new ParaShapeBase(self().downcast<ParaGeometry>());
}
