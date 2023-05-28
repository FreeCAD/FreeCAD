#include "AbsConstraint.h"

using namespace MbD;

std::shared_ptr<AbsConstraint> MbD::AbsConstraint::Create(const char* name)
{
	auto item = std::make_shared<AbsConstraint>(name);
	item->initialize();
	return item;
}

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
}
