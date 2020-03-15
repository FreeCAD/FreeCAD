#include "PreCompiled.h"

#include "ConstraintPlacementRules.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintPlacementRulesPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>
#include <src/Mod/ConstraintSolver/App/G2D/ParaPlacementPy.h>

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintPlacementRules, FCS::SimpleConstraint);


ConstraintPlacementRules::ConstraintPlacementRules()
    : placement(Py::None())
{
    initAttrs();
}

ConstraintPlacementRules::ConstraintPlacementRules(HParaPlacement plm)
    :ConstraintPlacementRules()
{
    this->placement = plm;
}

void ConstraintPlacementRules::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Child(reinterpret_cast<HParaObject &>(placement), "placement", &ParaPlacementPy::Type);
}

Base::DualNumber ConstraintPlacementRules::error1(const ValueSet& vals) const
{
    return (*placement->rotation)(vals).length() - 1.0;
}

PyObject* ConstraintPlacementRules::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintPlacementRulesPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
