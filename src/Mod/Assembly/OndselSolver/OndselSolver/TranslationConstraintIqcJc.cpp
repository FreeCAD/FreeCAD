/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "TranslationConstraintIqcJc.h"
#include "DispCompIeqcJecKeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIqcJc::TranslationConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi) :
	TranslationConstraintIJ(frmi, frmj, axisi)
{
}

void TranslationConstraintIqcJc::initriIeJeIe()
{
    riIeJeIe = CREATE<DispCompIeqcJecKeqc>::With(frmI, frmJ, frmI, axisI);
}

void TranslationConstraintIqcJc::calcPostDynCorrectorIteration()
{
	TranslationConstraintIJ::calcPostDynCorrectorIteration();
	auto riIeqJeIeq = std::static_pointer_cast<DispCompIeqcJecKeqc>(riIeJeIe);
	pGpXI = riIeqJeIeq->pvaluepXI();
	pGpEI = (riIeqJeIeq->pvaluepEI())->plusFullRow(riIeqJeIeq->pvaluepEK());
	ppGpXIpEI = riIeqJeIeq->ppvaluepXIpEK();
	ppGpEIpEI = riIeqJeIeq->ppvaluepEIpEI()
            ->plusFullMatrix(riIeqJeIeq->ppvaluepEIpEK())
            ->plusFullMatrix((riIeqJeIeq->ppvaluepEIpEK()->
                transpose()->plusFullMatrix(riIeqJeIeq->ppvaluepEKpEK())));
}

void TranslationConstraintIqcJc::useEquationNumbers()
{
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	iqXI = frmIeqc->iqX();
	iqEI = frmIeqc->iqE();
}

void TranslationConstraintIqcJc::fillPosICError(FColDsptr col)
{
	Constraint::fillPosICError(col);
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void TranslationConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
	auto ppGpXIpEIlam = ppGpXIpEI->times(lam);
	mat->atijplusFullMatrix(iqXI, iqEI, ppGpXIpEIlam);
	mat->atijplusTransposeFullMatrix(iqEI, iqXI, ppGpXIpEIlam);
	mat->atijplusFullMatrixtimes(iqEI, iqEI, ppGpEIpEI, lam);
}

void TranslationConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}

void TranslationConstraintIqcJc::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
}

void TranslationConstraintIqcJc::fillAccICIterError(FColDsptr col)
{
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto qXdotI = efrmIqc->qXdot();
	auto qEdotI = efrmIqc->qEdot();
	auto sum = pGpXI->timesFullColumn(efrmIqc->qXddot());
	sum += pGpEI->timesFullColumn(efrmIqc->qEddot());
	sum += 2.0 * (qXdotI->transposeTimesFullColumn(ppGpXIpEI->timesFullColumn(qEdotI)));
	sum += qEdotI->transposeTimesFullColumn(ppGpEIpEI->timesFullColumn(qEdotI));
	col->atiplusNumber(iG, sum);
}

void TranslationConstraintIqcJc::addToJointForceI(FColDsptr col)
{
	col->equalSelfPlusFullVectortimes(pGpXI, lam);
}

void TranslationConstraintIqcJc::addToJointTorqueI(FColDsptr jointTorque)
{
	auto cForceT = pGpXI->times(lam);
		auto rIpIeIp = frmI->rpep();
		auto pAOIppEI = frmI->pAOppE();
		auto aBOIp = frmI->aBOp();
		auto fpAOIppEIrIpIeIp = std::make_shared<FullColumn<double>>(4, 0.0);
		for (size_t i = 0; i < 4; i++)
		{
			auto dum = cForceT->timesFullColumn(pAOIppEI->at(i)->timesFullColumn(rIpIeIp));
			fpAOIppEIrIpIeIp->atiput(i, dum);
		}
		auto lampGpE = pGpEI->transpose()->times(lam);
		auto c2Torque = aBOIp->timesFullColumn(lampGpE->minusFullColumn(fpAOIppEIrIpIeIp));
		jointTorque->equalSelfPlusFullColumntimes(c2Torque, 0.5);
}
