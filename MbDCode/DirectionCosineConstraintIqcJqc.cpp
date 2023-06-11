#include "DirectionCosineConstraintIqcJqc.h"
#include "DirectionCosineIeqcJeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIqcJqc::DirectionCosineConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineConstraintIqcJc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqcJqc::initaAijIeJe()
{
	aAijIeJe = CREATE<DirectionCosineIeqcJeqc>::With(frmI, frmJ, axisI, axisJ);
}

void MbD::DirectionCosineConstraintIqcJqc::calcPostDynCorrectorIteration()
{
	DirectionCosineConstraintIqcJc::calcPostDynCorrectorIteration();
	auto aAijIeqJqe = std::static_pointer_cast<DirectionCosineIeqcJeqc>(aAijIeJe);
	pGpEJ = aAijIeqJqe->pAijIeJepEJ;
	ppGpEIpEJ = aAijIeqJqe->ppAijIeJepEIpEJ;
	ppGpEJpEJ = aAijIeqJqe->ppAijIeJepEJpEJ;
}

void MbD::DirectionCosineConstraintIqcJqc::useEquationNumbers()
{
	DirectionCosineConstraintIqcJc::useEquationNumbers();
	iqEJ = std::static_pointer_cast<EndFrameqc>(frmJ)->iqE();
}

void MbD::DirectionCosineConstraintIqcJqc::fillPosICError(FColDsptr col)
{
	DirectionCosineConstraintIqcJc::fillPosICError(col);
	col->atiplusFullVectortimes(iqEJ, pGpEJ, lam);
}

void MbD::DirectionCosineConstraintIqcJqc::fillPosICJacob(SpMatDsptr mat)
{
	DirectionCosineConstraintIqcJc::fillPosICJacob(mat);
	mat->atijplusFullRow(iG, iqEJ, pGpEJ);
	mat->atijplusFullColumn(iqEJ, iG, pGpEJ->transpose());
	auto ppGpEIpEJlam = ppGpEIpEJ->times(lam);
	mat->atijplusFullMatrix(iqEI, iqEJ, ppGpEIpEJlam);
	mat->atijplusTransposeFullMatrix(iqEJ, iqEI, ppGpEIpEJlam);
	mat->atijplusFullMatrixtimes(iqEJ, iqEJ, ppGpEJpEJ, lam);
}
