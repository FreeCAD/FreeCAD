/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <memory>

#include "ConstVelConstraintIqcJc.h"
#include "DirectionCosineIeqcJec.h"
#include "DirectionCosineIeqcJeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

MbD::ConstVelConstraintIqcJc::ConstVelConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj) : ConstVelConstraintIJ(frmi, frmj)
{
	pGpEI = std::make_shared<FullRow<double>>(4);
	ppGpEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::ConstVelConstraintIqcJc::calcPostDynCorrectorIteration()
{
	ConstVelConstraintIJ::calcPostDynCorrectorIteration();
	auto aA01IeqcJec = std::dynamic_pointer_cast<DirectionCosineIeqcJec>(aA01IeJe);
	auto& pA01IeJepEI = aA01IeqcJec->pAijIeJepEI;
	auto& ppA01IeJepEIpEI = aA01IeqcJec->ppAijIeJepEIpEI;
	auto aA10IeqcJec = std::dynamic_pointer_cast<DirectionCosineIeqcJec>(aA10IeJe);
	auto& pA10IeJepEI = aA10IeqcJec->pAijIeJepEI;
	auto& ppA10IeJepEIpEI = aA10IeqcJec->ppAijIeJepEIpEI;
	for (size_t i = 0; i < 4; i++)
	{
		pGpEI->atiput(i, pA01IeJepEI->at(i) + pA10IeJepEI->at(i));
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& ppGpEIpEIi = ppGpEIpEI->at(i);
		auto& ppA01IeJepEIpEIi = ppA01IeJepEIpEI->at(i);
		auto& ppA10IeJepEIpEIi = ppA10IeJepEIpEI->at(i);
		ppGpEIpEIi->atiput(i, ppA01IeJepEIpEIi->at(i) + ppA10IeJepEIpEIi->at(i));
		for (size_t j = i + 1; j < 4; j++)
		{
			auto ppGpEIpEIij = ppA01IeJepEIpEIi->at(j) + ppA10IeJepEIpEIi->at(j);
			ppGpEIpEIi->atiput(j, ppGpEIpEIij);
			ppGpEIpEI->atijput(j, i, ppGpEIpEIij);
		}
	}
}

void MbD::ConstVelConstraintIqcJc::fillAccICIterError(FColDsptr col)
{
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto qEdotI = efrmIqc->qEdot();
	double sum = 0.0;
	sum += pGpEI->timesFullColumn(efrmIqc->qEddot());
	sum += qEdotI->transposeTimesFullColumn(ppGpEIpEI->timesFullColumn(qEdotI));
	col->atiplusNumber(iG, sum);
}

void MbD::ConstVelConstraintIqcJc::fillPosICError(FColDsptr col)
{
	Constraint::fillPosICError(col);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::ConstVelConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
	mat->atijplusFullMatrixtimes(iqEI, iqEI, ppGpEIpEI, lam);
}

void MbD::ConstVelConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}

void MbD::ConstVelConstraintIqcJc::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
}

void MbD::ConstVelConstraintIqcJc::initA01IeJe()
{
	aA01IeJe = CREATE<DirectionCosineIeqcJec>::With(frmI, frmJ, 0, 1);
}

void MbD::ConstVelConstraintIqcJc::initA10IeJe()
{
	aA10IeJe = CREATE<DirectionCosineIeqcJec>::With(frmI, frmJ, 1, 0);
}

void MbD::ConstVelConstraintIqcJc::initialize()
{
	ConstVelConstraintIJ::initialize();
	pGpEI = std::make_shared<FullRow<double>>(4);
	ppGpEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::ConstVelConstraintIqcJc::useEquationNumbers()
{
	iqEI = std::static_pointer_cast<EndFrameqc>(frmI)->iqE();
}
