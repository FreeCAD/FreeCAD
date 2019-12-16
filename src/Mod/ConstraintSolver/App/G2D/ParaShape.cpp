#include "PreCompiled.h"

#include "ParaShape.h"
#include "G2D/ParaShapePy.h"

#include <Mod/ConstraintSolver/App/G2D/ParaTransformPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaShapeBase, FCS::ParaObject);


G2D::ParaShapeBase::ParaShapeBase()
    : placement(Py::None()), _tshape(Py::None())
{
    initAttrs();
}

G2D::ParaShapeBase::ParaShapeBase(HParaGeometry tshape, HParaTransform placement)
    : ParaShapeBase()
{
    this->_tshape = tshape;
    if (placement.isNone())
        this->placement = new ParaTransform();
    else
        this->placement = placement;
}

void FCS::G2D::ParaShapeBase::initAttrs()
{
    _attrs = {
    };
    _children = {
        //valueref , name       , PyType                , make , writeOnce
        {&placement, "placement", &ParaTransformPy::Type, false, true     },
        {&_tshape  , "tshape"   , &ParaTransformPy::Type, false, true     }
    };
}

std::string G2D::ParaShapeBase::repr() const
{
    std::stringstream ss;
    ss << "<" ;
    if (label.size() == 0)
        ss << "unlabeled ";
    ss << "2D shape " << shapeType().getName() ;
    if (label.size() != 0)
        ss << " '" << label << "'";
    ss << ">";
    return ss.str();
}

PyObject* G2D::ParaShapeBase::getPyObject()
{
    if (!_twin){
        new ParaShapePy(this);
        assert(_twin);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }

}

HParaObject FCS::G2D::ParaShapeBase::copy() const
{
    HParaShapeBase cpy = ParaObject::copy();
    cpy->placement = placement->copy();//copied shape has independent placement
    cpy->reversed = this->reversed;
    return cpy;
}

HParaShapeBase G2D::ParaShapeBase::getSubShape(const ParaGeometry& subshape)
{
    HParaShapeBase ret = new ParaShapeBase();
    ret->_tshape = const_cast<ParaGeometry&>(subshape).self();
    this->placement->lock();
    ret->placement = this->placement;
    ret->placement->lock();
    return ret;
}
