#include "AngleZConstraintIqcJqc.h"
#include "AngleZIeqcJeqc.h"
#include "EndFrameqc.h"

using namespace MbD;

MbD::AngleZConstraintIqcJqc::AngleZConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj) : AngleZConstraintIqcJc(frmi, frmj)
{
	pGpEJ = std::make_shared<FullRow<double>>(4);
	ppGpEIpEJ = std::make_shared<FullMatrix<double>>(4, 4);
	ppGpEJpEJ = std::make_shared<FullMatrix<double>>(4, 4);
}

void MbD::AngleZConstraintIqcJqc::initthezIeJe()
{
	thezIeJe = std::make_shared<AngleZIeqcJeqc>(frmI, frmJ);
}

void MbD::AngleZConstraintIqcJqc::calc_pGpEJ()
{
	pGpEJ = thezIeJe->pvaluepEJ();
}

void MbD::AngleZConstraintIqcJqc::calc_ppGpEIpEJ()
{
	ppGpEIpEJ = thezIeJe->ppvaluepEIpEJ();
}

void MbD::AngleZConstraintIqcJqc::calc_ppGpEJpEJ()
{
	ppGpEJpEJ = thezIeJe->ppvaluepEJpEJ();
}

void MbD::AngleZConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	AngleZConstraintIqcJc::calcPostDynCorrectorIteration();
	this->calc_pGpEJ();
	this->calc_ppGpEIpEJ();
	this->calc_ppGpEJpEJ();
}

void MbD::AngleZConstraintIqcJqc::fillAccICIterError(FColDsptr col)
{
	AngleZConstraintIqcJc::fillAccICIterError(col);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
	auto frmIeqc = std::static_pointer_cast<EndFrameqc>(frmI);
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qEdotI = frmIeqc->qEdot();
	auto qXdotJ = frmJeqc->qXdot();
	auto qEdotJ = frmJeqc->qEdot();
	double sum = 0.0;
	sum += pGpEJ->timesFullColumn(frmJeqc->qEddot());
	sum += 2.0 * (qEdotI->transposeTimesFullColumn(ppGpEIpEJ->timesFullColumn(qEdotJ)));
	sum += qEdotJ->transposeTimesFullColumn(ppGpEJpEJ->timesFullColumn(qEdotJ));
	col->atiplusNumber(iG, sum);
}

void MbD::AngleZConstraintIqcJqc::fillPosICError(FColDsptr col)
{
	AngleZConstraintIqcJc::fillPosICError(col);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
}

void MbD::AngleZConstraintIqcJqc::fillPosICJacob(SpMatDsptr mat)
{
	AngleZConstraintIqcJc::fillPosICJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
	auto ppGpEIpEJlam = ppGpEIpEJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqEJ, ppGpEIpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqEI, ppGpEIpEJlam);
	mat->atijplusFullMatrixtimes(iqEJ, iqEJ, ppGpEJpEJ, lam);
}

void MbD::AngleZConstraintIqcJqc::fillPosKineJacob(SpMatDsptr mat)
{
	AngleZConstraintIqcJc::fillPosKineJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
}

void MbD::AngleZConstraintIqcJqc::fillVelICJacob(SpMatDsptr mat)
{
	AngleZConstraintIqcJc::fillVelICJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
}

void MbD::AngleZConstraintIqcJqc::useEquationNumbers()
{
	AngleZConstraintIqcJc::useEquationNumbers();
	auto frmJeqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	iqEJ = frmJeqc->iqE();
}
