#include "DispCompIecJecO.h"

using namespace MbD;

DispCompIecJecO::DispCompIecJecO()
{
}

DispCompIecJecO::DispCompIecJecO(EndFrmcptr frmi, EndFrmcptr frmj, int axis) : KinematicIeJe(frmi, frmj), axis(axis)
{
}

void DispCompIecJecO::calcPostDynCorrectorIteration()
{
	riIeJeO = frmJ->riOeO(axis) - frmI->riOeO(axis);
}

FRowDsptr DispCompIecJecO::pvaluepXJ()
{
	assert(false);
	return FRowDsptr();
}

FRowDsptr DispCompIecJecO::pvaluepEJ()
{
	assert(false);
	return FRowDsptr();
}

FMatDsptr DispCompIecJecO::ppvaluepXJpEK()
{
	assert(false);
	return FMatDsptr();
}

FMatDsptr DispCompIecJecO::ppvaluepEJpEK()
{
	assert(false);
	return FMatDsptr();
}

FMatDsptr DispCompIecJecO::ppvaluepEJpEJ()
{
	assert(false);
	return FMatDsptr();
}
