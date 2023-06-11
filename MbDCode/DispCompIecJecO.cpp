#include "DispCompIecJecO.h"

using namespace MbD;

MbD::DispCompIecJecO::DispCompIecJecO()
{
}

MbD::DispCompIecJecO::DispCompIecJecO(EndFrmcptr frmi, EndFrmcptr frmj, int axis) : KinematicIeJe(frmi, frmj), axis(axis)
{
}

void MbD::DispCompIecJecO::calcPostDynCorrectorIteration()
{
	riIeJeO = frmJ->riOeO(axis) - frmI->riOeO(axis);
}

FRowDsptr MbD::DispCompIecJecO::pvaluepXJ()
{
	assert(false);
	return FRowDsptr();
}

FRowDsptr MbD::DispCompIecJecO::pvaluepEJ()
{
	assert(false);
	return FRowDsptr();
}

FMatDsptr MbD::DispCompIecJecO::ppvaluepXJpEK()
{
	assert(false);
	return FMatDsptr();
}

FMatDsptr MbD::DispCompIecJecO::ppvaluepEJpEK()
{
	assert(false);
	return FMatDsptr();
}

FMatDsptr MbD::DispCompIecJecO::ppvaluepEJpEJ()
{
	assert(false);
	return FMatDsptr();
}
