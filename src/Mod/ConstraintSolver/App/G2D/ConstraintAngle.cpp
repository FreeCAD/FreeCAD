#include "PreCompiled.h"

#include "ConstraintAngle.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintAnglePy.h"

#include "DualMath.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::G2D::ConstraintAngle, FCS::SimpleConstraint);


ConstraintAngle::ConstraintAngle()
{
    initAttrs();
}

void ConstraintAngle::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Parameter(angle, "angle", true, true, TURN/8);
}

Base::DualNumber ConstraintAngle::error1(const ValueSet& vals) const
{
    DualNumber ang = vals[angle];
    if (supplementAngle)
        ang = 0.5 * TURN - ang; //FIXME: sign-dependent?
    ang = ang * _revers;
    return signedAngle(calculateAngle(vals) - ang);
}

std::vector<Base::DualNumber> ConstraintAngle::caluclateDatum(const ValueSet& vals)
{
    throwIfIncomplete();
    DualNumber trueangle = signedAngle(calculateAngle(vals));
    DualNumber ang = _revers * trueangle ;
    if (supplementAngle)
        ang = 0.5 * TURN - ang;
    return {ang};
}

void ConstraintAngle::convertToSupplement(HValueSet vals)
{
    angle.throwNull();
    if (vals.isNone())
        vals = angle.host()->asValueSet().self();
    //if supplement:
    //    was: trueAngle = _revers * (0.5 * TURN - angle)
    //    new: trueAngle = _revers * (angle)
    //    trueAngle must not change.
    //    => angle = 0.5 * TURN - angle
    //if not supplement:
    //    was: trueAngle = _revers * (angle)
    //    new: trueAngle = _revers * (0.5 * TURN - angle)
    //    trueAngle must not change.
    //    => angle = 0.5 * TURN - angle. Same formula.
    vals->set(angle, 0.5 * TURN - (*vals)[angle]);
    supplementAngle = !supplementAngle;
}

void ConstraintAngle::convertToReversed(HValueSet vals)
{
    angle.throwNull();
    if (vals.isNone())
        vals = angle.host()->asValueSet().self();

    if (supplementAngle) {
        //was: trueAngle = _revers * (0.5 * TURN - angle)
        //new: trueAngle = _revers * -(0.5 * TURN - angle)
        //trueAngle must not change.
        vals->set(angle, TURN - (*vals)[angle]);
    } else {
        //was: trueAngle = _revers * angle
        //new: trueAngle = _revers * -angle
        //trueAngle must not change.
        vals->set(angle, -(*vals)[angle]);
    }
    setReversed(!isReversed());
}

PyObject* ConstraintAngle::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintAnglePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

HParaObject ConstraintAngle::copy() const
{
    HConstraintAngle ret = SimpleConstraint::copy().downcast<ConstraintAngle>();
    ret->supplementAngle = supplementAngle;
    return ret;
}
