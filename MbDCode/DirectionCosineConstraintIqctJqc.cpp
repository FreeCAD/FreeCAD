#include "DirectionCosineConstraintIqctJqc.h"
#include "DirectionCosineIeqctJeqc.h"
#include "CREATE.h"

using namespace MbD;

DirectionCosineConstraintIqctJqc::DirectionCosineConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	DirectionCosineConstraintIqcJqc(frmi, frmj, axisi, axisj)
{
}

void DirectionCosineConstraintIqctJqc::initaAijIeJe()
{
	aAijIeJe = CREATE<DirectionCosineIeqctJeqc>::With(frmI, frmJ, axisI, axisJ);
}

MbD::ConstraintType MbD::DirectionCosineConstraintIqctJqc::type()
{
	return MbD::essential;
}
