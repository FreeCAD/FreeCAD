#include "PreCompiled.h"

#include "ConstraintCurvePos.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintCurvePosPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintCurvePos, FCS::Constraint);


ConstraintCurvePos::ConstraintCurvePos()
    : point(Py::None()), curve(Py::None())
{
    initAttrs();
}

ConstraintCurvePos::ConstraintCurvePos(HParaCurve curve, ParameterRef u, HParaPoint p, std::string label)
    : ConstraintCurvePos()
{
    this->point = new ParaShape<ParaPoint>(new ParaTransform(), p);
    this->curve = new ParaShape<ParaCurve>(new ParaTransform(), curve);
    this->u = u;
    this->label = label;
}

ConstraintCurvePos::ConstraintCurvePos(HParaCurve curve, HParaPoint p, HParameterStore store, std::string label)
    : ConstraintCurvePos()
{
    this->point = new ParaShape<ParaPoint>(new ParaTransform(), p);
    this->curve = new ParaShape<ParaCurve>(new ParaTransform(), curve);
    this->makeParameters(store);
    this->label = label;
}

void ConstraintCurvePos::initAttrs()
{
    Constraint::initAttrs();

    tieAttr_Parameter(u, "u", true, true, 0.0);

    tieAttr_Shape(reinterpret_cast<HParaObject &>(point), "point", ParaPoint::getClassTypeId());
    tieAttr_Shape(reinterpret_cast<HParaObject &>(curve), "curve", ParaCurve::getClassTypeId());
}

void ConstraintCurvePos::error(const ValueSet& vals, Base::DualNumber* returnbuf) const
{
    Position pp = point->placement->value(vals) * point->tshape()(vals);
    Position pc = curve->placement->value(vals) * curve->tshape().value(vals, vals[u]);
    Vector diff = pp - pc;
    returnbuf[0] = diff.x;
    returnbuf[1] = diff.y;
}

void ConstraintCurvePos::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

std::vector<ParameterRef> ConstraintCurvePos::datumParameters() const
{
    return {u};
}

PyObject* ConstraintCurvePos::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintCurvePosPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
