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
    ParaObject::initAttrs();

    //                                                            make , req., writeonce
    tieAttr_Child(placement.upcast<ParaObject>(), "placement", &ParaTransformPy::Type, false, true, true);
    tieAttr_Child(_tshape.upcast<ParaObject>() , "tshape"   , &ParaTransformPy::Type, false, true, true);
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
    HParaShapeBase cpy = ParaObject::copy().downcast<ParaShapeBase>();
    cpy->placement = placement->copy().downcast<ParaTransform>();//copied shape has independent placement
    cpy->reversed = this->reversed;
    return cpy;
}

HParaShapeBase G2D::ParaShapeBase::getSubShape(const ParaGeometry& subshape)
{
    HParaShapeBase ret = new ParaShapeBase();
    ret->_tshape = const_cast<ParaGeometry&>(subshape).getHandle();
    this->placement->lock();
    ret->placement = this->placement;
    ret->placement->lock();
    return ret;
}
