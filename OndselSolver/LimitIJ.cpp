#include "LimitIJ.h"
#include "Constraint.h"

using namespace MbD;

MbD::LimitIJ::LimitIJ()
{
}

void MbD::LimitIJ::fillAccICIterError(FColDsptr col)
{
	assert(false);
}

void MbD::LimitIJ::fillAccICIterJacob(SpMatDsptr mat)
{
	assert(false);
}

bool MbD::LimitIJ::satisfied() const
{
	auto& constraint = constraints->front();
	if (type == "=<") {
		return constraint->aG < tol;
	}
	else if (type == "=>") {
		return constraint->aG > -tol;
	}
	assert(false);
	return true;
}

void MbD::LimitIJ::deactivate()
{
	active = false;
}

void MbD::LimitIJ::activate()
{
	active = true;
}

void MbD::LimitIJ::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	if (active) {
		ConstraintSet::fillConstraints(allConstraints);
	}
}

void MbD::LimitIJ::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	assert(false);
}

void MbD::LimitIJ::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	assert(false);
}

void MbD::LimitIJ::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	assert(false);
}

void MbD::LimitIJ::fillPosICError(FColDsptr col)
{
	if (active) {
		ConstraintSet::fillPosICError(col);
	}
}

void MbD::LimitIJ::fillPosICJacob(SpMatDsptr mat)
{
	if (active) {
		ConstraintSet::fillPosICJacob(mat);
	}
}

void MbD::LimitIJ::fillPosKineError(FColDsptr col)
{
	assert(false);
}

void MbD::LimitIJ::fillPosKineJacob(SpMatDsptr mat)
{
	assert(false);
}

void MbD::LimitIJ::fillqsuddotlam(FColDsptr col)
{
	assert(false);
}

void MbD::LimitIJ::fillqsulam(FColDsptr col)
{
	if (active) {
		ConstraintSet::fillqsulam(col);
	}
}

void MbD::LimitIJ::fillqsudot(FColDsptr col)
{
	assert(false);
}

void MbD::LimitIJ::fillqsudotWeights(DiagMatDsptr diagMat)
{
	assert(false);
}

void MbD::LimitIJ::fillVelICError(FColDsptr col)
{
	assert(false);
}

void MbD::LimitIJ::fillVelICJacob(SpMatDsptr mat)
{
	assert(false);
}

void MbD::LimitIJ::setqsuddotlam(FColDsptr col)
{
	assert(false);
}

void MbD::LimitIJ::setqsudotlam(FColDsptr col)
{
	assert(false);
}

void MbD::LimitIJ::setqsulam(FColDsptr col)
{
	if (active) {
		ConstraintSet::setqsulam(col);
	}
}

void MbD::LimitIJ::useEquationNumbers()
{
	if (active) {
		ConstraintSet::useEquationNumbers();
	}
}
