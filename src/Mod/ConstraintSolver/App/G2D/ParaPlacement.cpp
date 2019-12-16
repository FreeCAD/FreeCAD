#include "PreCompiled.h"

#include "ParaPlacement.h"
#include "G2D/ParaPlacementPy.h"

#include "G2D/ParaPointPy.h"
#include "G2D/ParaVectorPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaPlacement, FCS::ParaObject);

Placement ParaPlacement::operator()(const ValueSet& vals) const
{
    return Placement((*translation)(vals), (*rotation)(vals));
}

ParaPlacement::ParaPlacement()
    : translation(Py::None()), rotation(Py::None())
{
    initAttrs();
}

ParaPlacement::ParaPlacement(ParameterRef x, ParameterRef y, ParameterRef rx, ParameterRef ry)
    : ParaPlacement()
{
    this->translation = new ParaPoint(x,y);
    this->rotation = new ParaVector(rx,ry);
}

void FCS::G2D::ParaPlacement::initAttrs()
{
    _attrs = {
    };
    _children ={
        {&translation, "translation", &ParaPointPy::Type, true},
        {&rotation, "rotation", &ParaVectorPy::Type, true},
    };
}

PyObject* ParaPlacement::getPyObject()
{
    if (!_twin){
        new ParaPlacementPy(this);
        assert(_twin);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }

}

