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
	Constraint::initialize();
	pGpE = std::make_shared<FullRow<double>>(4);
}

void MbD::EulerConstraint::calcPostDynCorrectorIteration()
{
	auto& qE = static_cast<PartFrame*>(owner)->qE;
	aG = qE->sumOfSquares() - 1.0;
	for (int i = 0; i < 4; i++)
	{
		pGpE->at(i) = 2.0 * qE->at(i);
	}
}

void MbD::EulerConstraint::useEquationNumbers()
{
	iqE = static_cast<PartFrame*>(owner)->iqE;
}

void MbD::EulerConstraint::fillPosICError(FColDsptr col)
{
	Constraint::fillPosICError(col);
	col->atiplusFullVectortimes(iqE, pGpE, lam);
}

void MbD::EulerConstraint::fillPosICJacob(SpMatDsptr mat)
{
	//"ppGpEpE is a diag(2,2,2,2)."
	mat->atijplusFullRow(iG, iqE, pGpE);
	mat->atijplusFullColumn(iqE, iG, pGpE->transpose());
	auto twolam = 2.0 * lam;
	for (int i = 0; i < 4; i++)
	{
		auto ii = iqE + i;
		mat->atijplusNumber(ii, ii, twolam);
	}
}

void MbD::EulerConstraint::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqE, pGpE);
}
