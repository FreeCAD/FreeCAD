#include "DirectionCosineConstraintIqcJc.h"
#include "DirectionCosineIeqcJec.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIqcJc::DirectionCosineConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineConstraintIJ(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqcJc::initaAijIeJe()
{
	aAijIeJe = CREATE<DirectionCosineIeqcJec>::With(frmI, frmJ, axisI, axisJ);
}

void MbD::DirectionCosineConstraintIqcJc::calcPostDynCorrectorIteration()
{
	DirectionCosineConstraintIJ::calcPostDynCorrectorIteration();
	auto aAijIeqJe = std::static_pointer_cast<DirectionCosineIeqcJec>(aAijIeJe);
	pGpEI = aAijIeqJe->pAijIeJepEI;
	ppGpEIpEI = aAijIeqJe->ppAijIeJepEIpEI;
}

void MbD::DirectionCosineConstraintIqcJc::useEquationNumbers()
{
	iqEI = std::static_pointer_cast<EndFrameqc>(frmI)->iqE();
}

void MbD::DirectionCosineConstraintIqcJc::fillPosICError(FColDsptr col)
{
	Constraint::fillPosICError(col);
	col->atiplusFullVectortimes(iqEI, pGpEI, lam);
}

void MbD::DirectionCosineConstraintIqcJc::fillPosICJacob(SpMatDsptr mat)
{
	mat->atijplusFullRow(iG, iqEI, pGpEI);
	mat->atijplusFullColumn(iqEI, iG, pGpEI->transpose());
	mat->atijplusFullMatrixtimes(iqEI, iqEI, ppGpEIpEI, lam);
}
