#include "DirectionCosineConstraintIqcJqc.h"
#include "DirectionCosineIeqcJeqc.h"

using namespace MbD;

DirectionCosineConstraintIqcJqc::DirectionCosineConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineConstraintIqcJc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqcJqc::initialize()
{
}

void DirectionCosineConstraintIqcJqc::initaAijIeJe()
{
	aAijIeJe = std::make_shared<DirectionCosineIeqcJeqc>();
}

void MbD::DirectionCosineConstraintIqcJqc::calcPostDynCorrectorIteration()
{
}
