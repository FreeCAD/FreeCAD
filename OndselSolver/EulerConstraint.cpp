/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "EulerConstraint.h"
#include "Item.h"
#include "PartFrame.h"

using namespace MbD;

EulerConstraint::EulerConstraint()
{

}

EulerConstraint::EulerConstraint(const std::string& str) : Constraint(str)
{
}

void EulerConstraint::initialize()
{
	Constraint::initialize();
	pGpE = std::make_shared<FullRow<double>>(4);
}

void EulerConstraint::calcPostDynCorrectorIteration()
{
	auto& qE = static_cast<PartFrame*>(owner)->qE;
	aG = qE->sumOfSquares() - 1.0;
	for (size_t i = 0; i < 4; i++)
	{
		pGpE->at(i) = 2.0 * qE->at(i);
	}
}

void EulerConstraint::useEquationNumbers()
{
	iqE = static_cast<PartFrame*>(owner)->iqE;
}

void EulerConstraint::fillPosICError(FColDsptr col)
{
	Constraint::fillPosICError(col);
	col->atiplusFullVectortimes(iqE, pGpE, lam);
}

void EulerConstraint::fillPosICJacob(SpMatDsptr mat)
{
	//"ppGpEpE is a diag(2,2,2,2)."
	mat->atijplusFullRow(iG, iqE, pGpE);
	mat->atijplusFullColumn(iqE, iG, pGpE->transpose());
	auto twolam = 2.0 * lam;
	for (size_t i = 0; i < 4; i++)
	{
		auto ii = iqE + i;
		mat->atijplusNumber(ii, ii, twolam);
	}
}

void EulerConstraint::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqE, pGpE);
}

void EulerConstraint::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqE, pGpE);
	mat->atijplusFullColumn(iqE, iG, pGpE->transpose());
}

void EulerConstraint::fillAccICIterError(FColDsptr col)
{
	//"qdotT[ppGpqpq]*qdot."
	//"qdotT[2 2 2 2 diag]*qdot."

	col->atiplusFullVectortimes(iqE, pGpE, lam);
	auto partFrame = static_cast<PartFrame*>(owner);
	double sum = pGpE->timesFullColumn(partFrame->qEddot);
	sum += 2.0 * partFrame->qEdot->sumOfSquares();
	col->atiplusNumber(iG, sum);
}
