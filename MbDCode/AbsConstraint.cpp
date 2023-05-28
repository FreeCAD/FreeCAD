#include "AbsConstraint.h"
#include "PartFrame.h"

using namespace MbD;

AbsConstraint::AbsConstraint() {}

AbsConstraint::AbsConstraint(const char* str) : Constraint(str) {}

AbsConstraint::AbsConstraint(int i)
{
    axis = i;
}

void AbsConstraint::initialize()
{
    axis = 0;
    iqXminusOnePlusAxis = 0;
}

void MbD::AbsConstraint::calcPostDynCorrectorIteration()
{
    if (axis < 3) {
        aG = static_cast<PartFrame*>(owner)->qX->at(axis);
    }
    else {
        aG = static_cast<PartFrame*>(owner)->qE->at(axis - 3);
    }
}
