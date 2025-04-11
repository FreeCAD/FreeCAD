/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ConstVelConstraintIqcJqc.h"
#include "DirectionCosineIeqcJeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

MbD::ConstVelConstraintIqcJqc::ConstVelConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj) : ConstVelConstraintIqcJc(frmi, frmj)
{
	pGpEJ = std::make_shared<FullRow<double>>(4);
	ppGpEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppGpEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::ConstVelConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	ConstVelConstraintIqcJc::calcPostDynCorrectorIteration();
	auto aA01IeqcJeqc = std::dynamic_pointer_cast<DirectionCosineIeqcJeqc>(aA01IeJe);
	auto& pA01IeJepEJ = aA01IeqcJeqc->pAijIeJepEJ;
	auto& ppA01IeJepEIpEJ = aA01IeqcJeqc->ppAijIeJepEIpEJ;
	auto& ppA01IeJepEJpEJ = aA01IeqcJeqc->ppAijIeJepEJpEJ;
	auto aA10IeqcJeqc = std::dynamic_pointer_cast<DirectionCosineIeqcJeqc>(aA10IeJe);
	auto& pA10IeJepEJ = aA10IeqcJeqc->pAijIeJepEJ;
	auto& ppA10IeJepEIpEJ = aA10IeqcJeqc->ppAijIeJepEIpEJ;
	auto& ppA10IeJepEJpEJ = aA10IeqcJeqc->ppAijIeJepEJpEJ;
	for (size_t i = 0; i < 4; i++)
	{
		pGpEJ->atiput(i, pA01IeJepEJ->at(i) + pA10IeJepEJ->at(i));
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& ppGpEIpEJi = ppGpEIpEJ->at(i);
		auto& ppA01IeJepEIpEJi = ppA01IeJepEIpEJ->at(i);
		auto& ppA10IeJepEIpEJi = ppA10IeJepEIpEJ->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto ppGpEIpEJij = ppA01IeJepEIpEJi->at(j) + ppA10IeJepEIpEJi->at(j);
			ppGpEIpEJi->atiput(j, ppGpEIpEJij);
		}
	}
	for (size_t i = 0; i < 4; i++)
	{
		auto& ppGpEJpEJi = ppGpEJpEJ->at(i);
		auto& ppA01IeJepEJpEJi = ppA01IeJepEJpEJ->at(i);
		auto& ppA10IeJepEJpEJi = ppA10IeJepEJpEJ->at(i);
		ppGpEJpEJi->atiput(i, ppA01IeJepEJpEJi->at(i) + ppA10IeJepEJpEJi->at(i));
		for (size_t j = i + 1; j < 4; j++)
		{
			auto ppGpEJpEJij = ppA01IeJepEJpEJi->at(j) + ppA10IeJepEJpEJi->at(j);
			ppGpEJpEJi->atiput(j, ppGpEJpEJij);
			ppGpEJpEJ->atijput(j, i, ppGpEJpEJij);
		}
	}

}

void MbD::ConstVelConstraintIqcJqc::fillAccICIterError(FColDsptr col)
{
	ConstVelConstraintIqcJc::fillAccICIterError(col);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto qEdotI = efrmIqc->qEdot();
	auto efrmJqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qEdotJ = efrmJqc->qEdot();
	double sum = 0.0;
	sum += pGpEJ->timesFullColumn(efrmJqc->qEddot());
	sum += 2.0 * qEdotI->transposeTimesFullColumn(ppGpEIpEJ->timesFullColumn(qEdotJ));
	sum += qEdotJ->transposeTimesFullColumn(ppGpEJpEJ->timesFullColumn(qEdotJ));
	col->atiplusNumber(iG, sum);
}

void MbD::ConstVelConstraintIqcJqc::fillPosICError(FColDsptr col)
{
	ConstVelConstraintIqcJc::fillPosICError(col);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
}

void MbD::ConstVelConstraintIqcJqc::fillPosICJacob(SpMatDsptr mat)
{
	ConstVelConstraintIqcJc::fillPosICJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
	auto ppGpEIpEJlam = ppGpEIpEJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqEJ, ppGpEIpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqEI, ppGpEIpEJlam);
	mat->atijplusFullMatrixtimes(iqEJ, iqEJ, ppGpEJpEJ, lam);
}

void MbD::ConstVelConstraintIqcJqc::fillPosKineJacob(SpMatDsptr mat)
{
	ConstVelConstraintIqcJc::fillPosKineJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
}

void MbD::ConstVelConstraintIqcJqc::fillVelICJacob(SpMatDsptr mat)
{
	ConstVelConstraintIqcJc::fillVelICJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
}

void MbD::ConstVelConstraintIqcJqc::initA01IeJe()
{
	aA01IeJe = CREATE<DirectionCosineIeqcJeqc>::With(frmI, frmJ, 0, 1);
}

void MbD::ConstVelConstraintIqcJqc::initA10IeJe()
{
	aA10IeJe = CREATE<DirectionCosineIeqcJeqc>::With(frmI, frmJ, 1, 0);
}

void MbD::ConstVelConstraintIqcJqc::initialize()
{
	ConstVelConstraintIqcJc::initialize();
	pGpEJ = std::make_shared<FullRow<double>>(4);
	ppGpEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppGpEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::ConstVelConstraintIqcJqc::useEquationNumbers()
{
	ConstVelConstraintIqcJc::useEquationNumbers();
	iqEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqE();
}
