#include "PreCompiled.h"

#include "ConstraintPlacementRules.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintPlacementRulesPy.h"

#include <src/Mod/ConstraintSolver/App/G2D/ParaPointPy.h>

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
    _shapes = {
        {&placement, "placement", ParaPlacement::getClassTypeId()},
    };
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
