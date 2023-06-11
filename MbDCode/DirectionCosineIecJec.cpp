#include <memory>

#include "DirectionCosineIecJec.h"
#include "FullColumn.h"

using namespace MbD;

DirectionCosineIecJec::DirectionCosineIecJec()
{
}

DirectionCosineIecJec::DirectionCosineIecJec(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj) :
	KinematicIeJe(frmi, frmj), axisI(axisi), axisJ(axisj)
{

}

void MbD::DirectionCosineIecJec::calcPostDynCorrectorIteration()
{
	aAjOIe = frmI->aAjOe(axisI);
	aAjOJe = frmJ->aAjOe(axisJ);
	aAijIeJe = aAjOIe->dot(aAjOJe);
}

FRowDsptr MbD::DirectionCosineIecJec::pvaluepXJ()
{
	assert(false);
	return FRowDsptr();
}

FRowDsptr MbD::DirectionCosineIecJec::pvaluepEJ()
{
	assert(false);
	return FRowDsptr();
}

FMatDsptr MbD::DirectionCosineIecJec::ppvaluepXJpEK()
{
	assert(false);
	return FMatDsptr();
}

FMatDsptr MbD::DirectionCosineIecJec::ppvaluepEJpEK()
{
	assert(false);
	return FMatDsptr();
}

FMatDsptr MbD::DirectionCosineIecJec::ppvaluepEJpEJ()
{
	assert(false);
	return FMatDsptr();
}
