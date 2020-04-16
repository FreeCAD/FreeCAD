#include "PreCompiled.h"

#include "ParaHyperbola.h"
#include "G2D/ParaHyperbolaPy.h"

//#include "G2D/ConstraintHyperbolaRules.h"
#include "PyUtils.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaHyperbola, FCS::G2D::ParaConic);

ParaHyperbola::ParaHyperbola(bool full, bool bare)
    : majorAxisPoint(nullptr)
{
    _isFull = full;
    _bare = bare;
    initAttrs();
}

void FCS::G2D::ParaHyperbola::initAttrs()
{
    ParaConic::initAttrs();
    tieAttr_Parameter(radmin, "radmin", true, true, 0.5);
    //            attr                 name                   type                make    req.
    tieAttr_Child(center             , "center"             , &ParaPointPy::Type, true  , true );
    tieAttr_Child(focus1             , "focus1"             , &ParaPointPy::Type, !_bare, false);
    tieAttr_Child(focus2             , "focus2"             , &ParaPointPy::Type, !_bare, false);
    tieAttr_Child(minorDiameterLine  , "minorDiameterLine"  , &ParaPointPy::Type, !_bare, false);
    tieAttr_Child(majorDiameterLine  , "majorDiameterLine"  , &ParaPointPy::Type, !_bare, false);
    tieAttr_Child(majorAxisPoint     , "majorAxisPoint"     , &ParaPointPy::Type, true  , true );
}

std::vector<ParameterRef> ParaHyperbola::makeParameters(HParameterStore into)
{
    bool init_u0 = u0.isNull() && !_isFull;
    bool init_u1 = u1.isNull() && !_isFull;
    bool init_p0 = p0.isNone() && !_isFull;
    bool init_p1 = p1.isNone() && !_isFull;
    bool init_A = majorAxisPoint.isNone();
    std::vector<ParameterRef> ret = ParaConic::makeParameters(into);
    if (ret.size() > 0){
        ValueSet& vals = ret[0].host()->asValueSet();
        if (init_u0){
            u0.savedValue() = 0.0;
        }
        if (init_u1){
            u1.savedValue() = 1.0;
        }
        if (init_p0) {
            p0->setValue(vals, Position(1,0));
        }
        if (init_p1) {
            p0->setValue(vals, Position(2,1));
        }
        if (init_A) {
            majorAxisPoint->setValue(vals, Position(1,0));
        }
    }
    return ret;
}

Position ParaHyperbola::value(const ValueSet& vals, DualNumber u)
{
    Position c = center->value(vals);
    Vector xax = (majorAxisPoint->value(vals) - c).normalized();
    Vector yax = xax.rotate90ccw();
    return c + xax * getRMaj(vals) * cosh(u) + yax * getRMin(vals) * sinh(u);
}

Vector ParaHyperbola::tangent(const ValueSet& vals, DualNumber u)
{
    Position c = center->value(vals);
    Vector xax = (majorAxisPoint->value(vals) - c).normalized();
    Vector yax = xax.rotate90ccw();
    return c + xax * getRMaj(vals) * sinh(u) + yax * getRMin(vals) * cosh(u);
}

Vector ParaHyperbola::tangentAtXY(const ValueSet& vals, Position p)
{
    Position f1 = getFocus1(vals);
    Position f2 = getFocus2(vals);
    //tangent is a bisect between two lines from xy to foci.
    return (p - f1).normalized() + (p - f2).normalized();
}


DualNumber ParaHyperbola::pointOnCurveErrFunc(const ValueSet& vals, Position p)
{
    Position f1 = getFocus1(vals);
    Position f2 = getFocus2(vals);
    return (f2 - p).length() - (f1 - p).length() - 2 * getRMaj(vals);
}

std::vector<HConstraint> ParaHyperbola::makeRuleConstraints()
{
    std::vector<HConstraint> ret = ParaConic::makeRuleConstraints();
    //ret.push_back(new ConstraintHyperbolaRules(getHandle<ParaHyperbola>()));
    return ret;
}

Position ParaHyperbola::getFocus1(const ValueSet& vals) const
{
    Position c = center->value(vals);
    Vector xax = (majorAxisPoint->value(vals) - c).normalized();
    return c + xax * getF(vals);
}

//aka length "c"
DualNumber ParaHyperbola::getF(const ValueSet& vals) const
{
    DualNumber sqRMaj = (center->value(vals) - majorAxisPoint->value(vals)).sqlength();
    return sqrt(sqRMaj + sq(getRMin(vals)));
}

DualNumber ParaHyperbola::getRMaj(const ValueSet& vals) const
{
    return (center->value(vals) - majorAxisPoint->value(vals)).length();
}

DualNumber ParaHyperbola::getRMin(const ValueSet& vals) const
{
    return vals[radmin];
}

PyObject* ParaHyperbola::getPyObject()
{
    if (!_twin){
        _twin = new ParaHyperbolaPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}


