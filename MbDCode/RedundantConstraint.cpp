#include "RedundantConstraint.h"

using namespace MbD;

void RedundantConstraint::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
{
}

bool RedundantConstraint::isRedundant()
{
	return true;
}

std::string RedundantConstraint::classname()
{
	auto str = Item::classname() + "->" + constraint->classname();
	return str;
}

ConstraintType RedundantConstraint::type()
{
	return redundant;
}

void RedundantConstraint::fillqsulam(FColDsptr col)
{
}

void RedundantConstraint::postInput()
{
}

void RedundantConstraint::prePosIC()
{
}

void RedundantConstraint::fillEssenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
}

void RedundantConstraint::fillDispConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
}

void RedundantConstraint::fillPerpenConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
}

void RedundantConstraint::fillConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void RedundantConstraint::fillRedundantConstraints(std::shared_ptr<Constraint> sptr, std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
	redunConstraints->push_back(sptr);
}

void RedundantConstraint::setqsulam(FColDsptr col)
{
}

void RedundantConstraint::setqsudotlam(FColDsptr col)
{
}

void RedundantConstraint::fillPosICError(FColDsptr col)
{
}

void RedundantConstraint::fillPosKineError(FColDsptr col)
{
}

void RedundantConstraint::fillPosKineJacob(SpMatDsptr mat)
{
}

void RedundantConstraint::preVelIC()
{
}

void RedundantConstraint::preAccIC()
{
}

void RedundantConstraint::fillAccICIterError(FColDsptr col)
{
}

void RedundantConstraint::setqsuddotlam(FColDsptr col)
{
}

void RedundantConstraint::discontinuityAtaddTypeTo(double t, std::shared_ptr<std::vector<DiscontinuityType>> disconTypes)
{
	//"Reactivate all contraints."
	assert(false);
	//| newSelf |
	//newSelf : = self constraint.
	//newSelf discontinuityAt : tstartNew addTypeTo : collection.
	//self become : newSelf
}
