/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "GearConstraintIqcJqc.h"
#include "EndFrameqc.h"
#include "OrbitAngleZIeqcJeqc.h"
#include "CREATE.h"

using namespace MbD;

MbD::GearConstraintIqcJqc::GearConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj) : GearConstraintIqcJc(frmi, frmj)
{
}

void MbD::GearConstraintIqcJqc::calc_pGpEJ()
{
	pGpEJ = orbitJeIe->pvaluepEI()->plusFullRow(orbitIeJe->pvaluepEJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_pGpXJ()
{
	pGpXJ = orbitJeIe->pvaluepXI()->plusFullRow(orbitIeJe->pvaluepXJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_ppGpEIpEJ()
{
	ppGpEIpEJ = orbitJeIe->ppvaluepEIpEJ()->transpose()->plusFullMatrix(orbitIeJe->ppvaluepEIpEJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_ppGpEIpXJ()
{
	ppGpEIpXJ = orbitJeIe->ppvaluepXIpEJ()->transpose()->plusFullMatrix(orbitIeJe->ppvaluepEIpXJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_ppGpEJpEJ()
{
	ppGpEJpEJ = orbitJeIe->ppvaluepEIpEI()->plusFullMatrix(orbitIeJe->ppvaluepEJpEJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_ppGpXIpEJ()
{
	ppGpXIpEJ = orbitJeIe->ppvaluepEIpXJ()->transpose()->plusFullMatrix(orbitIeJe->ppvaluepXIpEJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_ppGpXIpXJ()
{
	ppGpXIpXJ = orbitJeIe->ppvaluepXIpXJ()->transpose()->plusFullMatrix(orbitIeJe->ppvaluepXIpXJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_ppGpXJpEJ()
{
	ppGpXJpEJ = orbitJeIe->ppvaluepXIpEI()->plusFullMatrix(orbitIeJe->ppvaluepXJpEJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calc_ppGpXJpXJ()
{
	ppGpXJpXJ = orbitJeIe->ppvaluepXIpXI()->plusFullMatrix(orbitIeJe->ppvaluepXJpXJ()->times(this->ratio()));
}

void MbD::GearConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	GearConstraintIqcJc::calcPostDynCorrectorIteration();
	this->calc_pGpXJ();
	this->calc_pGpEJ();
	this->calc_ppGpXIpXJ();
	this->calc_ppGpXIpEJ();
	this->calc_ppGpEIpXJ();
	this->calc_ppGpEIpEJ();
	this->calc_ppGpXJpXJ();
	this->calc_ppGpXJpEJ();
	this->calc_ppGpEJpEJ();
}

void MbD::GearConstraintIqcJqc::fillAccICIterError(FColDsptr col)
{
	GearConstraintIqcJc::fillAccICIterError(col);
	col->atiplusFullVectortimes(iqXJ, pGpXJ, lam);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qXdotI = frmIeqc->qXdot();
	auto qEdotI = frmIeqc->qEdot();
	auto qXdotJ = frmJeqc->qXdot();
	auto qEdotJ = frmJeqc->qEdot();
	double sum = 0.0;
	sum += pGpXJ->timesFullColumn(frmJeqc->qXddot());
	sum += pGpEJ->timesFullColumn(frmJeqc->qEddot());
	sum += 2.0 * (qXdotI->transposeTimesFullColumn(ppGpXIpXJ->timesFullColumn(qXdotJ)));
	sum += 2.0 * (qEdotI->transposeTimesFullColumn(ppGpEIpXJ->timesFullColumn(qXdotJ)));
	sum += qXdotJ->transposeTimesFullColumn(ppGpXJpXJ->timesFullColumn(qXdotJ));
	sum += 2.0 * (qXdotI->transposeTimesFullColumn(ppGpXIpEJ->timesFullColumn(qEdotJ)));
	sum += 2.0 * (qEdotI->transposeTimesFullColumn(ppGpEIpEJ->timesFullColumn(qEdotJ)));
	sum += 2.0 * (qXdotJ->transposeTimesFullColumn(ppGpXJpEJ->timesFullColumn(qEdotJ)));
	sum += qEdotJ->transposeTimesFullColumn(ppGpEJpEJ->timesFullColumn(qEdotJ));
	col->atiplusNumber(iG, sum);
}

void MbD::GearConstraintIqcJqc::fillPosICError(FColDsptr col)
{
	GearConstraintIqcJc::fillPosICError(col);
	col->atiplusFullVectortimes(iqXJ, pGpXJ, lam);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
}

void MbD::GearConstraintIqcJqc::fillPosICJacob(SpMatDsptr mat)
{
	GearConstraintIqcJc::fillPosICJacob(mat);
	mat->atijplusFullRow(iG, iqXJ, pGpXJ);
	mat->atijplusFullColumn(iqXJ, iG, pGpXJ->transpose());
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
	auto ppGpXIpXJlam = ppGpXIpXJ->times(lam);
	mat->atijplusFullMatrix(iqXI, iqXJ, ppGpXIpXJlam);
	mat->atijplusTransposeFullMatrix(iqXJ, iqXI, ppGpXIpXJlam);
	auto ppGpEIpXJlam = ppGpEIpXJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqXJ, ppGpEIpXJlam);
	mat->atijplusTransposeFullMatrix(iqXJ, iqEI, ppGpEIpXJlam);
	mat->atijplusFullMatrixtimes(iqXJ, iqXJ, ppGpXJpXJ, lam);
	auto ppGpXIpEJlam = ppGpXIpEJ->times(lam);
	mat->atijplusFullMatrix(iqXI, iqEJ, ppGpXIpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqXI, ppGpXIpEJlam);
	auto ppGpEIpEJlam = ppGpEIpEJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqEJ, ppGpEIpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqEI, ppGpEIpEJlam);
	auto ppGpXJpEJlam = ppGpXJpEJ->times(lam);
	mat->atijplusFullMatrix(iqXJ, iqEJ, ppGpXJpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqXJ, ppGpXJpEJlam);
	mat->atijplusFullMatrixtimes(iqEJ, iqEJ, ppGpEJpEJ, lam);
}

void MbD::GearConstraintIqcJqc::fillPosKineJacob(SpMatDsptr mat)
{
	GearConstraintIqcJc::fillPosKineJacob(mat);
	mat->atijplusFullRow(iG, iqXJ, pGpXJ);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
}

void MbD::GearConstraintIqcJqc::fillVelICJacob(SpMatDsptr mat)
{
	GearConstraintIqcJc::fillVelICJacob(mat);
	mat->atijplusFullRow(iG, iqXJ, pGpXJ);
	mat->atijplusFullColumn(iqXJ, iG, pGpXJ->transpose());
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
}

void MbD::GearConstraintIqcJqc::initorbitsIJ()
{
	orbitIeJe = CREATE<OrbitAngleZIeqcJeqc>::With(frmI, frmJ);
	orbitJeIe = CREATE<OrbitAngleZIeqcJeqc>::With(frmJ, frmI);
}

void MbD::GearConstraintIqcJqc::useEquationNumbers()
{
	GearConstraintIqcJc::useEquationNumbers();
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	iqXJ = frmJeqc->iqX();
	iqEJ = frmJeqc->iqE();
}
