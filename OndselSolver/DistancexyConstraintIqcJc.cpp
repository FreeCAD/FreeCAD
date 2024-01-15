/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DistancexyConstraintIqcJc.h"
#include "EndFramec.h"
#include "CREATE.h"
#include "DispCompIeqcJecIe.h"

using namespace MbD;

MbD::DistancexyConstraintIqcJc::DistancexyConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj) : DistancexyConstraintIJ(frmi, frmj)
{
}

void MbD::DistancexyConstraintIqcJc::addToJointForceI(FColDsptr col)
{
	col->equalSelfPlusFullVectortimes(pGpXI, lam);
}

void MbD::DistancexyConstraintIqcJc::addToJointTorqueI(FColDsptr jointTorque)
{
	auto cForceT = pGpXI->times(lam);
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto rIpIeIp = frmIeqc->rpep();
	auto pAOIppEI = frmIeqc->pAOppE();
	auto aBOIp = frmIeqc->aBOp();
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

void MbD::DistancexyConstraintIqcJc::calcPostDynCorrectorIteration()
{
	DistancexyConstraintIJ::calcPostDynCorrectorIteration();
	this->calc_pGpXI();
	this->calc_pGpEI();
	this->calc_ppGpXIpXI();
	this->calc_ppGpXIpEI();
	this->calc_ppGpEIpEI();
}

void MbD::DistancexyConstraintIqcJc::calc_pGpEI()
{
	pGpEI = (xIeJeIe->pvaluepEI()->times(xIeJeIe->value())->plusFullRow(yIeJeIe->pvaluepEI()->times(yIeJeIe->value())));
	pGpEI->magnifySelf(2.0);
}

void MbD::DistancexyConstraintIqcJc::calc_pGpXI()
{
	pGpXI = (xIeJeIe->pvaluepXI()->times(xIeJeIe->value())->plusFullRow(yIeJeIe->pvaluepXI()->times(yIeJeIe->value())));
	pGpXI->magnifySelf(2.0);
}

void MbD::DistancexyConstraintIqcJc::calc_ppGpEIpEI()
{
	ppGpEIpEI = (xIeJeIe->pvaluepEI()->transposeTimesFullRow(xIeJeIe->pvaluepEI()));
	ppGpEIpEI = ppGpEIpEI->plusFullMatrix(xIeJeIe->ppvaluepEIpEI()->times(xIeJeIe->value()));
	ppGpEIpEI = ppGpEIpEI->plusFullMatrix(yIeJeIe->pvaluepEI()->transposeTimesFullRow(yIeJeIe->pvaluepEI()));
	ppGpEIpEI = ppGpEIpEI->plusFullMatrix(yIeJeIe->ppvaluepEIpEI()->times(yIeJeIe->value()));
	ppGpEIpEI->magnifySelf(2.0);
}

void MbD::DistancexyConstraintIqcJc::calc_ppGpXIpEI()
{
	ppGpXIpEI = (xIeJeIe->pvaluepXI()->transposeTimesFullRow(xIeJeIe->pvaluepEI()));
	ppGpXIpEI = ppGpXIpEI->plusFullMatrix(xIeJeIe->ppvaluepXIpEI()->times(xIeJeIe->value()));
	ppGpXIpEI = ppGpXIpEI->plusFullMatrix(yIeJeIe->pvaluepXI()->transposeTimesFullRow(yIeJeIe->pvaluepEI()));
	ppGpXIpEI = ppGpXIpEI->plusFullMatrix(yIeJeIe->ppvaluepXIpEI()->times(yIeJeIe->value()));
	ppGpXIpEI->magnifySelf(2.0);
}

void MbD::DistancexyConstraintIqcJc::calc_ppGpXIpXI()
{
	//xIeJeIe ppvaluepXIpXI = 0
	//yIeJeIe ppvaluepXIpXI = 0
	ppGpXIpXI = (xIeJeIe->pvaluepXI()->transposeTimesFullRow(xIeJeIe->pvaluepXI()));
	ppGpXIpXI = ppGpXIpXI->plusFullMatrix(yIeJeIe->pvaluepXI()->transposeTimesFullRow(yIeJeIe->pvaluepXI()));
	ppGpXIpXI->magnifySelf(2.0);
}

void MbD::DistancexyConstraintIqcJc::init_xyIeJeIe()
{
	xIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 0);
	yIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 1);
}

void MbD::DistancexyConstraintIqcJc::fillAccICIterError(FColDsptr col)
{
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto qXdotI = efrmIqc->qXdot();
	auto qEdotI = efrmIqc->qEdot();
	auto sum = pGpXI->timesFullColumn(efrmIqc->qXddot());
	sum += pGpEI->timesFullColumn(efrmIqc->qEddot());
	sum += qXdotI->transposeTimesFullColumn(ppGpXIpXI->timesFullColumn(qXdotI));
	sum += 2.0 * (qXdotI->transposeTimesFullColumn(ppGpXIpEI->timesFullColumn(qEdotI)));
	sum += qEdotI->transposeTimesFullColumn(ppGpEIpEI->timesFullColumn(qEdotI));
	col->atiplusNumber(iG, sum);
}

void MbD::DistancexyConstraintIqcJc::fillPosICError(FColDsptr col)
{
	DistancexyConstraintIJ::fillPosICError(col);
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::DistancexyConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
	mat->atijplusFullMatrixtimes(iqXI, iqXI, ppGpXIpXI, lam);
	auto ppGpXIpEIlam = ppGpXIpEI->times(lam);
	mat->atijplusFullMatrix(iqXI, iqEI, ppGpXIpEIlam);
	mat->atijplusTransposeFullMatrix(iqEI, iqXI, ppGpXIpEIlam);
	mat->atijplusFullMatrixtimes(iqEI, iqEI, ppGpEIpEI, lam);
}

void MbD::DistancexyConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}

void MbD::DistancexyConstraintIqcJc::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
}

void MbD::DistancexyConstraintIqcJc::useEquationNumbers()
{
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	iqXI = frmIeqc->iqX();
	iqEI = frmIeqc->iqE();
}
