#include "DispCompIecJecO.h"

using namespace MbD;

MbD::DispCompIecJecO::DispCompIecJecO()
{
}

MbD::DispCompIecJecO::DispCompIecJecO(EndFrmcptr frmi, EndFrmcptr frmj, size_t axis) : KinematicIeJe(frmi, frmj), axis(axis)
{
}

void MbD::DispCompIecJecO::calcPostDynCorrectorIteration()
{
	riIeJeO = frmJ->riOeO(axis) - frmI->riOeO(axis);
}
