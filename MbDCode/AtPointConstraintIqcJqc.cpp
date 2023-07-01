#include "AtPointConstraintIqcJqc.h"
#include "DispCompIeqcJeqcO.h"
#include "CREATE.h"
#include "EndFrameqc.h"

using namespace MbD;

AtPointConstraintIqcJqc::AtPointConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	AtPointConstraintIqcJc(frmi, frmj, axisi)
{
}

void AtPointConstraintIqcJqc::initializeGlobally()
{
	AtPointConstraintIqcJc::initializeGlobally();
	ppGpEJpEJ = (std::static_pointer_cast<DispCompIeqcJeqcO>(riIeJeO))->ppriIeJeOpEJpEJ;
}

void AtPointConstraintIqcJqc::initriIeJeO()
{
	riIeJeO = CREATE<DispCompIeqcJeqcO>::With(frmI, frmJ, axis);
}

void AtPointConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	AtPointConstraintIqcJc::calcPostDynCorrectorIteration();
	pGpEJ = std::static_pointer_cast<DispCompIeqcJeqcO>(riIeJeO)->priIeJeOpEJ;
}

void AtPointConstraintIqcJqc::useEquationNumbers()
{
	AtPointConstraintIqcJc::useEquationNumbers();
	iqXJminusOnePlusAxis = std::static_pointer_cast<EndFrameqc>(frmJ)->iqX() + axis;
	iqEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqE();
}

void AtPointConstraintIqcJqc::fillPosICError(FColDsptr col)
{
	AtPointConstraintIqcJc::fillPosICError(col);
	col->at(iqXJminusOnePlusAxis) += lam;
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
}

void AtPointConstraintIqcJqc::fillPosICJacob(SpMatDsptr mat)
{
	AtPointConstraintIqcJc::fillPosICJacob(mat);
	mat->atijplusNumber(iG, iqXJminusOnePlusAxis, 1.0);
	mat->atijplusNumber(iqXJminusOnePlusAxis, iG, 1.0);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
	mat->atijplusFullMatrixtimes(iqEJ, iqEJ, ppGpEJpEJ, lam);
}

void AtPointConstraintIqcJqc::fillPosKineJacob(SpMatDsptr mat)
{
	AtPointConstraintIqcJc::fillPosKineJacob(mat);
	mat->atijplusNumber(iG, iqXJminusOnePlusAxis, 1.0);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
}

void AtPointConstraintIqcJqc::fillVelICJacob(SpMatDsptr mat)
{
	AtPointConstraintIqcJc::fillVelICJacob(mat);
	mat->atijplusNumber(iG, iqXJminusOnePlusAxis, 1.0);
	mat->atijplusNumber(iqXJminusOnePlusAxis, iG, 1.0);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
}

void AtPointConstraintIqcJqc::fillAccICIterError(FColDsptr col)
{
	AtPointConstraintIqcJc::fillAccICIterError(col);
	col->atiplusNumber(iqXJminusOnePlusAxis, lam);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
	auto efrmJqc = std::static_pointer_cast<EndFrameqc>(frmJ);
	auto qEdotJ = efrmJqc->qEdot();
	auto sum = efrmJqc->qXddot()->at(axis);
	sum += pGpEJ->timesFullColumn(efrmJqc->qEddot());
	sum += qEdotJ->transposeTimesFullColumn(ppGpEJpEJ->timesFullColumn(qEdotJ));
	col->atiplusNumber(iG, sum);
}
