#include "DirectionCosineConstraintIqctJqc.h"
#include "DirectionCosineIeqctJeqc.h"

using namespace MbD;

DirectionCosineConstraintIqctJqc::DirectionCosineConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineConstraintIqcJqc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqctJqc::initialize()
{
}

void DirectionCosineConstraintIqctJqc::initaAijIeJe()
{
	aAijIeJe = std::make_shared<DirectionCosineIeqctJeqc>();
}

void MbD::DirectionCosineConstraintIqctJqc::calcPostDynCorrectorIteration()
{
}
