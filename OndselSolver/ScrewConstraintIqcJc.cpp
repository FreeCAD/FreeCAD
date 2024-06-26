/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <cmath>

#include "ScrewConstraintIqcJc.h"
#include "EndFrameqc.h"
#include "CREATE.h"
#include "DispCompIeqcJecIe.h"
#include "AngleZIeqcJec.h"

using namespace MbD;

MbD::ScrewConstraintIqcJc::ScrewConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj) : ScrewConstraintIJ(frmi, frmj)
{
	pGpXI = std::make_shared<FullRow<double>>(3);
	pGpEI = std::make_shared<FullRow<double>>(4);
	ppGpXIpEI = std::make_shared<FullMatrix<double>>(3, 4);
	ppGpEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::ScrewConstraintIqcJc::initzIeJeIe()
{
	zIeJeIe = std::make_shared<DispCompIeqcJecIe>(frmI, frmJ, 2);
}

void MbD::ScrewConstraintIqcJc::initthezIeJe()
{
	thezIeJe = std::make_shared<AngleZIeqcJec>(frmI, frmJ);
}

void MbD::ScrewConstraintIqcJc::addToJointForceI(FColDsptr col)
{
	col->equalSelfPlusFullVectortimes(pGpXI, lam);
}

void MbD::ScrewConstraintIqcJc::addToJointTorqueI(FColDsptr jointTorque)
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

void MbD::ScrewConstraintIqcJc::calc_pGpEI()
{
	pGpEI = zIeJeIe->pvaluepEI()->times(2.0 * M_PI)->minusFullRow(thezIeJe->pvaluepEI()->times(pitch));
}

void MbD::ScrewConstraintIqcJc::calc_pGpXI()
{
	pGpXI = zIeJeIe->pvaluepXI()->times(2.0 * M_PI);
}

void MbD::ScrewConstraintIqcJc::calc_ppGpEIpEI()
{
	ppGpEIpEI = zIeJeIe->ppvaluepEIpEI()->times(2.0 * M_PI)
		->minusFullMatrix(thezIeJe->ppvaluepEIpEI()->times(pitch));
}

void MbD::ScrewConstraintIqcJc::calc_ppGpXIpEI()
{
	ppGpXIpEI = zIeJeIe->ppvaluepXIpEI()->times(2.0 * M_PI);
}

void MbD::ScrewConstraintIqcJc::calcPostDynCorrectorIteration()
{
	ScrewConstraintIJ::calcPostDynCorrectorIteration();
	this->calc_pGpXI();
	this->calc_pGpEI();
	this->calc_ppGpXIpEI();
	this->calc_ppGpEIpEI();
}

void MbD::ScrewConstraintIqcJc::fillAccICIterError(FColDsptr col)
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

void MbD::ScrewConstraintIqcJc::fillPosICError(FColDsptr col)
{
	ScrewConstraintIJ::fillPosICError(col);
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::ScrewConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
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

void MbD::ScrewConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}

void MbD::ScrewConstraintIqcJc::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
}

void MbD::ScrewConstraintIqcJc::init_zthez()
{
	zIeJeIe = CREATE<DispCompIeqcJecIe>::With(frmI, frmJ, 2);
	thezIeJe = CREATE<AngleZIeqcJec>::With(frmI, frmJ);
}

void MbD::ScrewConstraintIqcJc::useEquationNumbers()
{
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	iqXI = frmIeqc->iqX();
	iqEI = frmIeqc->iqE();
}
