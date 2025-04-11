#include "AngleZConstraintIqcJc.h"
#include "AngleZIeqcJec.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::AngleZConstraintIqcJc::AngleZConstraintIqcJc(EndFrmsptr frmi, EndFrmsptr frmj) : AngleZConstraintIJ(frmi, frmj)
{
	pGpEI = std::make_shared<FullRow<double>>(4);
	ppGpEIpEI = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::AngleZConstraintIqcJc::initthezIeJe()
{
	thezIeJe = std::make_shared<AngleZIeqcJec>(frmI, frmJ);
}

void MbD::AngleZConstraintIqcJc::addToJointTorqueI(FColDsptr jointTorque)
{
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto rIpIeIp = frmIeqc->rpep();
	auto pAOIppEI = frmIeqc->pAOppE();
	auto aBOIp = frmIeqc->aBOp();
	auto fpAOIppEIrIpIeIp = std::make_shared<FullColumn<double>>(4, 0.0);
	auto lampGpE = pGpEI->transpose()->times(lam);
	auto c2Torque = aBOIp->timesFullColumn(lampGpE->minusFullColumn(fpAOIppEIrIpIeIp));
	jointTorque->equalSelfPlusFullColumntimes(c2Torque, 0.5);
}

void MbD::AngleZConstraintIqcJc::calc_pGpEI()
{
	pGpEI = thezIeJe->pvaluepEI();
}

void MbD::AngleZConstraintIqcJc::calc_ppGpEIpEI()
{
	ppGpEIpEI = thezIeJe->ppvaluepEIpEI();
}

void MbD::AngleZConstraintIqcJc::calcPostDynCorrectorIteration()
{
	AngleZConstraintIJ::calcPostDynCorrectorIteration();
	this->calc_pGpEI();
	this->calc_ppGpEIpEI();
}

void MbD::AngleZConstraintIqcJc::fillAccICIterError(FColDsptr col)
{
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
	auto efrmIqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto qXdotI = efrmIqc->qXdot();
	auto qEdotI = efrmIqc->qEdot();
	auto sum = pGpEI->timesFullColumn(efrmIqc->qEddot());
	sum += qEdotI->transposeTimesFullColumn(ppGpEIpEI->timesFullColumn(qEdotI));
	col->atiplusNumber(iG, sum);
}

void MbD::AngleZConstraintIqcJc::fillPosICError(FColDsptr col)
{
	AngleZConstraintIJ::fillPosICError(col);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::AngleZConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
	mat->atijplusFullMatrixtimes(iqEI, iqEI, ppGpEIpEI, lam);
}

void MbD::AngleZConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}

void MbD::AngleZConstraintIqcJc::fillVelICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
}

void MbD::AngleZConstraintIqcJc::useEquationNumbers()
{
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	iqEI = frmIeqc->iqE();
}
