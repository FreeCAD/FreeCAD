#include "PreCompiled.h"

#include "ParaEllipse.h"
#include "G2D/ParaEllipsePy.h"
#include "PyUtils.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaEllipse, FCS::G2D::ParaConic);

ParaEllipse::ParaEllipse(bool full, bool bare)
{
    _isFull = full;
    _bare = bare;
    initAttrs();
}

void FCS::G2D::ParaEllipse::initAttrs()
{
    ParaConic::initAttrs();
    tieAttr_Parameter(radmin, "radmin", true, true, 0.5);
    //            attr                 name                   type                make    req.
    tieAttr_Child(center             , "center"             , &ParaPointPy::Type, true  , true );
    tieAttr_Child(focus1             , "focus1"             , &ParaPointPy::Type, true  , true );
    tieAttr_Child(focus2             , "focus2"             , &ParaPointPy::Type, !_bare, false);
    tieAttr_Child(minorDiameterLine  , "minorDiameterLine"  , &ParaPointPy::Type, !_bare, false);
    tieAttr_Child(majorDiameterLine  , "majorDiameterLine"  , &ParaPointPy::Type, !_bare, false);
}

std::vector<ParameterRef> ParaEllipse::makeParameters(HParameterStore into)
{
    bool init_u0 = u0.isNull() && !_isFull;
    bool init_u1 = u1.isNull() && !_isFull;
    bool init_p0 = p0.isNone() && !_isFull;
    bool init_p1 = p1.isNone() && !_isFull;
    bool init_f1 = focus1.isNone();
    std::vector<ParameterRef> ret = ParaConic::makeParameters(into);
    if (ret.size() > 0){
        ValueSet& vals = ret[0].host()->asValueSet();
        if (init_u0){
            u0.savedValue() = 0.0;
        }
        if (init_u1){
            u1.savedValue() = TURN/4;
        }
        if (init_p0) {
            p0->setValue(vals, Position(1,0));
        }
        if (init_p1) {
            p0->setValue(vals, Position(0,0.5));
        }
        if (init_f1) {
            focus1->setValue(vals, Position(0.7,0));
        }
    }
    return ret;
}

Position ParaEllipse::value(const ValueSet& vals, DualNumber u)
{
    Position c = center->value(vals);
    Vector xax = (focus1->value(vals) - c).normalized();
    Vector yax = xax.rotate90ccw();
    return c + xax * getRMaj(vals) * cos(u) + yax * getRMin(vals) * sin(u);
}

Vector ParaEllipse::tangent(const ValueSet& vals, DualNumber u)
{
    Position c = center->value(vals);
    Vector xax = (focus1->value(vals) - c).normalized();
    Vector yax = xax.rotate90ccw();
    return c - xax * getRMaj(vals) * sin(u) + yax * getRMin(vals) * cos(u);
}

Vector ParaEllipse::tangentAtXY(const ValueSet& vals, Position p)
{
    Position f1 = getFocus1(vals);
    Position f2 = getFocus2(vals);
    //normal is a bisect between two lines from xy to foci. Tangent is normal, rotated.
    Vector normal = (f1 - p).normalized() + (f2 - p).normalized();
    return normal.rotate90cw();
}


DualNumber ParaEllipse::pointOnCurveErrFunc(const ValueSet& vals, Position p)
{
    Position f1 = getFocus1(vals);
    Position f2 = getFocus2(vals);
    return (f1 - p).length() + (f2 - p).length() - 2 * getRMaj(vals);
}

std::vector<HConstraint> ParaEllipse::makeRuleConstraints()
{
    throw Py::NotImplementedError("ParaEllipse::makeRuleConstraints");
}

Position ParaEllipse::getFocus1(const ValueSet& vals) const
{
    return focus1->value(vals);
}

DualNumber ParaEllipse::getF(const ValueSet& vals) const
{
    return (center->value(vals) - focus1->value(vals)).length();
}

DualNumber ParaEllipse::getRMaj(const ValueSet& vals) const
{
    DualNumber sqf = (focus1->value(vals) - center->value(vals)).sqlength();
    return sqrt(sqf + sq(vals[radmin]));
}

DualNumber ParaEllipse::getRMin(const ValueSet& vals) const
{
    return vals[radmin];
}

PyObject* ParaEllipse::getPyObject()
{
    if (!_twin){
        _twin = new ParaEllipsePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}


