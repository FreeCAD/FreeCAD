#include "PreCompiled.h"

#include "SimpleConstraint.h"

using namespace FCS;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::SimpleConstraint, FCS::Constraint);

void FCS::SimpleConstraint::error(const FCS::ValueSet& vals, Base::DualNumber* returnbuf) const
{
    returnbuf[0] = error1(vals);
}
