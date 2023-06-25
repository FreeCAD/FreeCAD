#include "RedundantConstraint.h"

using namespace MbD;

void MbD::RedundantConstraint::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
{
}

bool MbD::RedundantConstraint::isRedundant()
{
	return true;
}

std::string MbD::RedundantConstraint::classname()
{
	auto str = Item::classname() + "->" + constraint->classname();
	return str;
}

MbD::ConstraintType MbD::RedundantConstraint::type()
{
	return MbD::redundant;
}

void MbD::RedundantConstraint::fillqsulam(FColDsptr col)
{
}

void MbD::RedundantConstraint::postInput()
{
}

void MbD::RedundantConstraint::prePosIC()
{
}

void MbD::RedundantConstraint::fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
}

void MbD::RedundantConstraint::fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
}

void MbD::RedundantConstraint::fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
}

void MbD::RedundantConstraint::fillConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void MbD::RedundantConstraint::fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
	redunConstraints->push_back(sptr);
}

void MbD::RedundantConstraint::setqsulam(FColDsptr col)
{
}

void MbD::RedundantConstraint::setqsudotlam(FColDsptr col)
{
}

void MbD::RedundantConstraint::fillPosICError(FColDsptr col)
{
}

void MbD::RedundantConstraint::fillPosKineError(FColDsptr col)
{
}

void MbD::RedundantConstraint::fillPosKineJacob(SpMatDsptr mat)
{
}

void MbD::RedundantConstraint::preVelIC()
{
}

void MbD::RedundantConstraint::preAccIC()
{
}

void MbD::RedundantConstraint::fillAccICIterError(FColDsptr col)
{
}

void MbD::RedundantConstraint::setqsuddotlam(FColDsptr qsudotlam)
{
}
