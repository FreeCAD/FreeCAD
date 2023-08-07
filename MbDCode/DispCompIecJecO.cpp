#include "DispCompIecJecO.h"
#include "EndFramec.h"

using namespace MbD;

DispCompIecJecO::DispCompIecJecO()
{
}

DispCompIecJecO::DispCompIecJecO(EndFrmsptr frmi, EndFrmsptr frmj, int axis) : KinematicIeJe(frmi, frmj), axis(axis)
{
}

void DispCompIecJecO::calcPostDynCorrectorIteration()
{
	riIeJeO = frmJ->riOeO(axis) - frmI->riOeO(axis);
}

double MbD::DispCompIecJecO::value()
{
	return riIeJeO;
}
