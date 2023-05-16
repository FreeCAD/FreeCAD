#include "EulerConstraint.h"
#include "Item.h"

using namespace MbD;

EulerConstraint::EulerConstraint()
{
	initialize();
}

EulerConstraint::EulerConstraint(const char* str) : Constraint(str)
{
}

void EulerConstraint::initialize()
{
	pGpE = std::make_shared<FullRow<double>>(4);
}
