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

double MbD::DispCompIecJecO::value()
{
	return riIeJeO;
}
