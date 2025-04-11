/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DirectionCosineConstraintIqcJqc.h"
#include "DirectionCosineIeqcJeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIqcJqc::DirectionCosineConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj) :
	DirectionCosineConstraintIqcJc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqcJqc::initaAijIeJe()
{
	aAijIeJe = CREATE<DirectionCosineIeqcJeqc>::With(frmI, frmJ, axisI, axisJ);
}

void DirectionCosineConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	DirectionCosineConstraintIqcJc::calcPostDynCorrectorIteration();
	auto aAijIeqJqe = std::static_pointer_cast<DirectionCosineIeqcJeqc>(aAijIeJe);
	pGpEJ = aAijIeqJqe->pAijIeJepEJ;
	ppGpEIpEJ = aAijIeqJqe->ppAijIeJepEIpEJ;
	ppGpEJpEJ = aAijIeqJqe->ppAijIeJepEJpEJ;
}

void DirectionCosineConstraintIqcJqc::useEquationNumbers()
{
	DirectionCosineConstraintIqcJc::useEquationNumbers();
	iqEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqE();
}

void DirectionCosineConstraintIqcJqc::fillPosICError(FColDsptr col)
{
	DirectionCosineConstraintIqcJc::fillPosICError(col);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
}

void DirectionCosineConstraintIqcJqc::fillPosICJacob(SpMatDsptr mat)
{
	DirectionCosineConstraintIqcJc::fillPosICJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
	auto ppGpEIpEJlam = ppGpEIpEJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqEJ, ppGpEIpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqEI, ppGpEIpEJlam);
	mat->atijplusFullMatrixtimes(iqEJ, iqEJ, ppGpEJpEJ, lam);
}

void DirectionCosineConstraintIqcJqc::fillPosKineJacob(SpMatDsptr mat)
{
	DirectionCosineConstraintIqcJc::fillPosKineJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
}

void DirectionCosineConstraintIqcJqc::fillVelICJacob(SpMatDsptr mat)
{
	DirectionCosineConstraintIqcJc::fillVelICJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
}

void DirectionCosineConstraintIqcJqc::fillAccICIterError(FColDsptr col)
{
	DirectionCosineConstraintIqcJc::fillAccICIterError(col);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto efrmJqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qEdotI = efrmIqc->qEdot();
	auto qEdotJ = efrmJqc->qEdot();
	double sum = pGpEJ->timesFullColumn(efrmJqc->qEddot());
	sum += (qEdotI->transposeTimesFullColumn(ppGpEIpEJ->timesFullColumn(qEdotJ))) * 2.0;
	sum += qEdotJ->transposeTimesFullColumn(ppGpEJpEJ->timesFullColumn(qEdotJ));
	col->atiplusNumber(iG, sum);
}
