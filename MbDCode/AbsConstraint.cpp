#include "AbsConstraint.h"

using namespace MbD;

AbsConstraint::AbsConstraint()
{
    initialize();
}

AbsConstraint::AbsConstraint(const char* str) : Constraint(str)
{
    initialize();
}

MbD::AbsConstraint::AbsConstraint(int i)
{
    axis = i;
}

void AbsConstraint::initialize()
{
    axis = 0;
    iqXminusOnePlusAxis = 0;
}
