#include "PreCompiled.h"

#include "ConstraintSnellsLawAtXY.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintSnellsLawAtXYPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>
#include <src/Mod/ConstraintSolver/App/G2D/ParaPlacementPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintSnellsLawAtXY, FCS::SimpleConstraint);


ConstraintSnellsLawAtXY::ConstraintSnellsLawAtXY()
    : ray1(Py::None()), ray2(Py::None()), boundary(Py::None()), p(Py::None())
{
    initAttrs();
}

void ConstraintSnellsLawAtXY::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(ray1, "crv1", ParaCurve::getClassTypeId());
    tieAttr_Shape(ray2, "crv2", ParaCurve::getClassTypeId());
    tieAttr_Shape(boundary, "boundary", ParaCurve::getClassTypeId());
    tieAttr_Shape(p, "p", ParaLine::getClassTypeId());
    tieAttr_Parameter(n1, "n1", true, true, 1.0);
    tieAttr_Parameter(n2, "n2", true, true, 1.0);
}

Base::DualNumber ConstraintSnellsLawAtXY::error1(const ValueSet& vals) const
{
    DualNumber sin1, sin2;
    calculateSines(vals, sin1, sin2);
    return vals[n1] * sin1 - vals[n2] * sin2;
}

std::vector<Base::DualNumber> ConstraintSnellsLawAtXY::calculateDatum(const ValueSet& vals)
{
    throwIfIncomplete();

    DualNumber sin1, sin2;
    calculateSines(vals, sin1, sin2);
    DualNumber n1v = vals[n1];
    DualNumber n2v = vals[n2];
    if (abs(n1v) < 1e-10)
        n1v = 1.0;
    if (abs(n2v) < 1e-10)
        n2v = 1.0;
    if (abs(sin1) < 1e-10 || abs(sin2) < 1e-10)
        throw Py::ValueError("One of the angles is normal, can't compute refractive indices");
    if (abs(sin2) > abs(sin1)){
        n1v = n2v * sin2 / sin1;
    } else {
        n2v = n1v * sin1 / sin2;
    }
    return {n1v, n2v};
}

HParaObject ConstraintSnellsLawAtXY::copy() const
{
    HConstraintSnellsLawAtXY cpy = SimpleConstraint::copy().downcast<ConstraintSnellsLawAtXY>();
    cpy->flipt1 = flipt1;
    cpy->flipt2 = flipt2;
    return cpy;
}

PyObject* ConstraintSnellsLawAtXY::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintSnellsLawAtXYPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

void ConstraintSnellsLawAtXY::calculateSines(const ValueSet& vals, DualNumber& out_sin1, DualNumber& out_sin2) const
{
    Placement plm1 = ray1->placement->value(vals);
    Placement plm2 = ray2->placement->value(vals);
    Placement plmb = boundary->placement->value(vals);
    Position pv = p->placement->value(vals) * p->tshape().value(vals);
    Vector t1 = plm1 * ray1->tshape().tangentAtXY(vals, plm1.inverse() * pv).normalized();
    Vector t2 = plm1 * ray2->tshape().tangentAtXY(vals, plm2.inverse() * pv).normalized();
    Vector btang = plmb * boundary->tshape().tangentAtXY(vals, plmb.inverse() * pv).rotate90ccw().normalized();
    if (flipt1)
        t1 = -t1;
    if (flipt2)
        t2 = -t2;
    //sine of angle of incidence is dot product between boundary tangent and ray tangent
    out_sin1 = Vector::dot(t1, btang);
    out_sin2 = Vector::dot(t2, btang);
}
