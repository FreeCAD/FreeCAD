/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "RackPinConstraintIqcJqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"
#include "DispCompIeqcJeqcIe.h"
#include "AngleZIeqcJeqc.h"

using namespace MbD;

MbD::RackPinConstraintIqcJqc::RackPinConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj) : RackPinConstraintIqcJc(frmi, frmj)
{
	pGpXJ = std::make_shared<FullRow<double>>(3);
	pGpEJ = std::make_shared<FullRow<double>>(4);
	ppGpEIpXJ = std::make_shared<FullMatrix<double>>(4, 3);
	ppGpEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppGpEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::RackPinConstraintIqcJqc::initxIeJeIe()
{
	xIeJeIe = std::make_shared<DispCompIeqcJeqcIe>(frmI, frmJ, 0);
}

void MbD::RackPinConstraintIqcJqc::initthezIeJe()
{
	thezIeJe = std::make_shared<AngleZIeqcJeqc>(frmI, frmJ);
}

void MbD::RackPinConstraintIqcJqc::calc_pGpEJ()
{
	pGpEJ = xIeJeIe->pvaluepEJ()->plusFullRow(thezIeJe->pvaluepEJ()->times(pitchRadius));
}

void MbD::RackPinConstraintIqcJqc::calc_pGpXJ()
{
	pGpXJ = xIeJeIe->pvaluepXJ();
}

void MbD::RackPinConstraintIqcJqc::calc_ppGpEIpEJ()
{
	ppGpEIpEJ = xIeJeIe->ppvaluepEIpEJ()
            ->plusFullMatrix(thezIeJe->ppvaluepEIpEJ()->times(pitchRadius));
}

void MbD::RackPinConstraintIqcJqc::calc_ppGpEIpXJ()
{
	ppGpEIpXJ = xIeJeIe->ppvaluepEIpXJ();
}

void MbD::RackPinConstraintIqcJqc::calc_ppGpEJpEJ()
{
	ppGpEJpEJ = xIeJeIe->ppvaluepEJpEJ()
            ->plusFullMatrix(thezIeJe->ppvaluepEJpEJ()->times(pitchRadius));
}

void MbD::RackPinConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	RackPinConstraintIqcJc::calcPostDynCorrectorIteration();
	this->calc_pGpXJ();
	this->calc_pGpEJ();
	this->calc_ppGpEIpXJ();
	this->calc_ppGpEIpEJ();
	this->calc_ppGpEJpEJ();
}

void MbD::RackPinConstraintIqcJqc::fillAccICIterError(FColDsptr col)
{
	RackPinConstraintIqcJc::fillAccICIterError(col);
	col->atiplusFullVectortimes(iqXJ, pGpXJ, lam);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qEdotI = frmIeqc->qEdot();
	auto qXdotJ = frmJeqc->qXdot();
	auto qEdotJ = frmJeqc->qEdot();
	double sum = 0.0;
	sum += pGpXJ->timesFullColumn(frmJeqc->qXddot());
	sum += pGpEJ->timesFullColumn(frmJeqc->qEddot());
	sum += 2.0 * (qEdotI->transposeTimesFullColumn(ppGpEIpXJ->timesFullColumn(qXdotJ)));
	sum += 2.0 * (qEdotI->transposeTimesFullColumn(ppGpEIpEJ->timesFullColumn(qEdotJ)));
	sum += qEdotJ->transposeTimesFullColumn(ppGpEJpEJ->timesFullColumn(qEdotJ));
	col->atiplusNumber(iG, sum);
}

void MbD::RackPinConstraintIqcJqc::fillPosICError(FColDsptr col)
{
	RackPinConstraintIqcJc::fillPosICError(col);
	col->atiplusFullVectortimes(iqXJ, pGpXJ, lam);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
}

void MbD::RackPinConstraintIqcJqc::fillPosICJacob(SpMatDsptr mat)
{
	RackPinConstraintIqcJc::fillPosICJacob(mat);
	mat->atijplusFullRow(iG, iqXJ, pGpXJ);
	mat->atijplusFullColumn(iqXJ, iG, pGpXJ->transpose());
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
	auto ppGpEIpXJlam = ppGpEIpXJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqXJ, ppGpEIpXJlam);
	mat->atijplusTransposeFullMatrix(iqXJ, iqEI, ppGpEIpXJlam);
	auto ppGpEIpEJlam = ppGpEIpEJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqEJ, ppGpEIpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqEI, ppGpEIpEJlam);
	mat->atijplusFullMatrixtimes(iqEJ, iqEJ, ppGpEJpEJ, lam);
}

void MbD::RackPinConstraintIqcJqc::fillPosKineJacob(SpMatDsptr mat)
{
	RackPinConstraintIqcJc::fillPosKineJacob(mat);
	mat->atijplusFullRow(iG, iqXJ, pGpXJ);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
}

void MbD::RackPinConstraintIqcJqc::fillVelICJacob(SpMatDsptr mat)
{
	RackPinConstraintIqcJc::fillVelICJacob(mat);
	mat->atijplusFullRow(iG, iqXJ, pGpXJ);
	mat->atijplusFullColumn(iqXJ, iG, pGpXJ->transpose());
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
}

void MbD::RackPinConstraintIqcJqc::init_xthez()
{
	xIeJeIe = CREATE<DispCompIeqcJeqcIe>::With(frmI, frmJ, 0);
	thezIeJe = CREATE<AngleZIeqcJeqc>::With(frmI, frmJ);
}

void MbD::RackPinConstraintIqcJqc::useEquationNumbers()
{
	RackPinConstraintIqcJc::useEquationNumbers();
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	iqXJ = frmJeqc->iqX();
	iqEJ = frmJeqc->iqE();
}
