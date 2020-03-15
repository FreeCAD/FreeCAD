#include "PreCompiled.h"

#include "ConstraintLength.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintLengthPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>
#include <src/Mod/ConstraintSolver/App/G2D/ParaPlacementPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintLength, FCS::SimpleConstraint);


ConstraintLength::ConstraintLength()
{
    initAttrs();
}

ConstraintLength::ConstraintLength(std::vector<HShape_Curve> edges, ParameterRef length)
    : ConstraintLength()
{
    setEdges(edges);
    this->length = length;
}

void ConstraintLength::setEdges(const std::vector<HShape_Curve> edges)
{
    _edges = edges;
    touch();
}

void ConstraintLength::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Parameter(length, "length", true, true, 1.0);
}

HParaObject ConstraintLength::copy() const
{
    HConstraintLength cpy = SimpleConstraint::copy().downcast<ConstraintLength>();
    cpy->_edges = this->_edges;
    return cpy;

}

void ConstraintLength::forEachShape(std::function<void (const ParaObject::ShapeRef&)> callback) const
{
    int i = 0;
    for (const HShape_Curve& crv : _edges){
        ParaObject::ShapeRef ref;
        ref.name = "Edges[" + std::to_string(i) + "]";
        ref.value = reinterpret_cast<HParaObject *>(const_cast<HShape_Curve*>(&crv)); //FIXME: avoid const-cast somehow
        ref.type = ParaCurve::getClassTypeId();
        callback(ref);
        ++i;
    }
}

void ConstraintLength::setWeight(double weight)
{
    SimpleConstraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

DualNumber ConstraintLength::calculateLength(const ValueSet& vals) const
{
    DualNumber cum = 0;
    for (const HShape_Curve& crv : _edges){
        cum = cum + crv->tshape().length(vals);
    }
    return cum;
}

Base::DualNumber ConstraintLength::error1(const ValueSet& vals) const
{
    return calculateLength(vals) - vals[length];
}

std::vector<Base::DualNumber> ConstraintLength::caluclateDatum(const ValueSet& vals)
{
    throwIfIncomplete();
    return {calculateLength(vals)};
}

PyObject* ConstraintLength::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintLengthPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
