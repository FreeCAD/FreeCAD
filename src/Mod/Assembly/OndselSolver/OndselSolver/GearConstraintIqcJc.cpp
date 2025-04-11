/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "GearConstraintIqcJc.h"
#include "EndFrameqc.h"
#include "CREATE.h"
#include "OrbitAngleZIeqcJec.h"

using namespace MbD;

MbD::GearConstraintIqcJc::GearConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj) : GearConstraintIJ(frmi, frmj)
{
}

void MbD::GearConstraintIqcJc::addToJointForceI(FColDsptr col)
{
	col->equalSelfPlusFullVectortimes(pGpXI, lam);
}

void MbD::GearConstraintIqcJc::addToJointTorqueI(FColDsptr jointTorque)
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

void MbD::GearConstraintIqcJc::calc_pGpEI()
{
	pGpEI = orbitJeIe->pvaluepEJ()->plusFullRow(orbitIeJe->pvaluepEI()->times(this->ratio()));
}

void MbD::GearConstraintIqcJc::calc_pGpXI()
{
	pGpXI = orbitJeIe->pvaluepXJ()->plusFullRow(orbitIeJe->pvaluepXI()->times(this->ratio()));
}

void MbD::GearConstraintIqcJc::calc_ppGpEIpEI()
{
	ppGpEIpEI = orbitJeIe->ppvaluepEJpEJ()->plusFullMatrix(orbitIeJe->ppvaluepEIpEI()->times(this->ratio()));
}

void MbD::GearConstraintIqcJc::calc_ppGpXIpEI()
{
	ppGpXIpEI = orbitJeIe->ppvaluepXJpEJ()->plusFullMatrix(orbitIeJe->ppvaluepXIpEI()->times(this->ratio()));
}

void MbD::GearConstraintIqcJc::calc_ppGpXIpXI()
{
	ppGpXIpXI = orbitJeIe->ppvaluepXJpXJ()
            ->plusFullMatrix(orbitIeJe->ppvaluepXIpXI()->times(this->ratio()));
}

void MbD::GearConstraintIqcJc::calcPostDynCorrectorIteration()
{
	GearConstraintIJ::calcPostDynCorrectorIteration();
	this->calc_pGpXI();
	this->calc_pGpEI();
	this->calc_ppGpXIpXI();
	this->calc_ppGpXIpEI();
	this->calc_ppGpEIpEI();
}

void MbD::GearConstraintIqcJc::fillAccICIterError(FColDsptr col)
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

void MbD::GearConstraintIqcJc::fillPosICError(FColDsptr col)
{
	GearConstraintIJ::fillPosICError(col);
	col->atiplusFullVectortimes(iqXI, pGpXI, lam);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::GearConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
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

void MbD::GearConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}

void MbD::GearConstraintIqcJc::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqXI, pGpXI);
	mat->atijplusFullColumn(iqXI, iG, pGpXI->transpose());
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
}

void MbD::GearConstraintIqcJc::initorbitsIJ()
{
	orbitIeJe = CREATE<OrbitAngleZIeqcJec>::With(frmI, frmJ);
	orbitJeIe = CREATE<OrbitAngleZIeqcJec>::With(frmJ, frmI);
}

void MbD::GearConstraintIqcJc::useEquationNumbers()
{
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	iqXI = frmIeqc->iqX();
	iqEI = frmIeqc->iqE();
}
