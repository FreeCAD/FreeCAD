#include "DirectionCosineConstraintIqcJc.h"
#include "DirectionCosineIeqcJec.h"

using namespace MbD;

DirectionCosineConstraintIqcJc::DirectionCosineConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineConstraintIJ(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqcJc::initialize()
{
}

void DirectionCosineConstraintIqcJc::initaAijIeJe()
{
	aAijIeJe = std::make_shared<DirectionCosineIeqcJec>();
}

void MbD::DirectionCosineConstraintIqcJc::calcPostDynCorrectorIteration()
{
}
