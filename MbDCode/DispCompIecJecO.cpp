#include "DispCompIecJecO.h"

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
