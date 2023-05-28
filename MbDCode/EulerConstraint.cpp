#include "EulerConstraint.h"
#include "Item.h"

using namespace MbD;

std::shared_ptr<EulerConstraint> MbD::EulerConstraint::Create()
{
	auto item = std::make_shared<EulerConstraint>();
	item->initialize();
	return item;
}

EulerConstraint::EulerConstraint()
{

}

EulerConstraint::EulerConstraint(const char* str) : Constraint(str)
{
}

void EulerConstraint::initialize()
{
	pGpE = std::make_shared<FullRow<double>>(4);
}

void MbD::EulerConstraint::calcPostDynCorrectorIteration()
{
}
