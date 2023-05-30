#include "EulerConstraint.h"
#include "Item.h"
#include "PartFrame.h"

using namespace MbD;

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
	auto& qE = static_cast<PartFrame*>(owner)->qE;
	aG = qE->sumOfSquares() - 1.0;
	for (size_t i = 0; i < 4; i++)
	{
		pGpE->at(i) = 2.0 * qE->at(i);
	}
}