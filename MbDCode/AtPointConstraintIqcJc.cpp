#include "AtPointConstraintIqcJc.h"
#include "DispCompIeqcJecO.h"
#include "CREATE.h"
#include "EndFrameqc.h"

using namespace MbD;

AtPointConstraintIqcJc::AtPointConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	AtPointConstraintIJ(frmi, frmj, axisi)
{
}

void MbD::AtPointConstraintIqcJc::initializeGlobally()
{
	AtPointConstraintIJ::initializeGlobally();
	ppGpEIpEI = (std::static_pointer_cast<DispCompIeqcJecO>(riIeJeO))->ppriIeJeOpEIpEI;
}

void AtPointConstraintIqcJc::initriIeJeO()
{
	riIeJeO = CREATE<DispCompIeqcJecO>::With(frmI, frmJ, axis);
}

void MbD::AtPointConstraintIqcJc::calcPostDynCorrectorIteration()
{
	AtPointConstraintIJ::calcPostDynCorrectorIteration();
	pGpEI = std::static_pointer_cast<DispCompIeqcJecO>(riIeJeO)->priIeJeOpEI;
}

void MbD::AtPointConstraintIqcJc::useEquationNumbers()
{
	iqXIminusOnePlusAxis = std::static_pointer_cast<EndFrameqc>(frmI)->iqX() + axis;
	iqEI = std::static_pointer_cast<EndFrameqc>(frmI)->iqE();
}

void MbD::AtPointConstraintIqcJc::fillPosICError(FColDsptr col)
{
	Constraint::fillPosICError(col);
	col->at(iqXIminusOnePlusAxis) -= lam;
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::AtPointConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
{
	mat->atijplusNumber(iG, iqXIminusOnePlusAxis, -1.0);
	mat->atijplusNumber(iqXIminusOnePlusAxis, iG, -1.0);
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
	mat->atijplusFullMatrixtimes(iqEI, iqEI, ppGpEIpEI, lam);
}

void MbD::AtPointConstraintIqcJc::fillPosKineJacob(SpMatDsptr mat)
{
	mat->atijplusNumber(iG, iqXIminusOnePlusAxis, -1.0);
	mat->atijplusFullRow(iG, iqEI, pGpEI);
}
