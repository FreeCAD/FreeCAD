#include "DirectionCosineConstraintIqcJqc.h"
#include "DirectionCosineIeqcJeqc.h"
#include "EndFrameqc.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIqcJqc::DirectionCosineConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj) :
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
