/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "RackPinConstraintIqcJc.h"
#include "EndFrameqc.h"
#include "CREATE.h"
#include "AngleZIeqcJec.h"
#include "DispCompIeqcJecIe.h"

using namespace MbD;

MbD::RackPinConstraintIqcJc::RackPinConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj) : RackPinConstraintIJ(frmi, frmj)
{
	pGpXI = std::make_shared<FullRow<double>>(3);
	pGpEI = std::make_shared<FullRow<double>>(4);
	ppGpXIpEI = std::make_shared<FullMatrix<double>>(3, 4);
	ppGpEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::RackPinConstraintIqcJc::initxIeJeIe()
{
	xIeJeIe = std::make_shared<DispCompIeqcJecIe>(frmI, frmJ, 0);
}

void MbD::RackPinConstraintIqcJc::initthezIeJe()
{
	thezIeJe = std::make_shared<AngleZIeqcJec>(frmI, frmJ);
}

void MbD::RackPinConstraintIqcJc::addToJointForceI(FColDsptr col)
{
	col->equalSelfPlusFullVectortimes(pGpXI, lam);
}

void MbD::RackPinConstraintIqcJc::addToJointTorqueI(FColDsptr jointTorque)
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

void MbD::RackPinConstraintIqcJc::calc_pGpEI()
{
	pGpEI = xIeJeIe->pvaluepEI()->plusFullRow(thezIeJe->pvaluepEI()->times(pitchRadius));
}

void MbD::RackPinConstraintIqcJc::calc_pGpXI()
{
	pGpXI = xIeJeIe->pvaluepXI();
}

void MbD::RackPinConstraintIqcJc::calc_ppGpEIpEI()
{
	ppGpEIpEI = xIeJeIe->ppvaluepEIpEI()
            ->plusFullMatrix(thezIeJe->ppvaluepEIpEI()->times(pitchRadius));
}

void MbD::RackPinConstraintIqcJc::calc_ppGpXIpEI()
{
	ppGpXIpEI = xIeJeIe->ppvaluepXIpEI();
}

void MbD::RackPinConstraintIqcJc::calcPostDynCorrectorIteration()
{
	RackPinConstraintIJ::calcPostDynCorrectorIteration();
	this->calc_pGpXI();
	this->calc_pGpEI();
	this->calc_ppGpXIpEI();
	this->calc_ppGpEIpEI();
}

void MbD::RackPinConstraintIqcJc::fillAccICIterError(FColDsptr col)
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

void MbD::RackPinConstraintIqcJc::fillPosICError(FColDsptr col)
{
	RackPinConstraintIJ::fillPosICError(col);
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::RackPinConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
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

void MbD::RackPinConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}

void MbD::RackPinConstraintIqcJc::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
}

void MbD::RackPinConstraintIqcJc::init_xthez()
{
	xIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 0);
	thezIeJe = CREATE<AngleZIeqcJec>::With(frmI, frmJ);
}

void MbD::RackPinConstraintIqcJc::useEquationNumbers()
{
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	iqXI = frmIeqc->iqX();
	iqEI = frmIeqc->iqE();
}
