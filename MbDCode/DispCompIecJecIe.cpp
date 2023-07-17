#include "DispCompIecJecIe.h"

using namespace MbD;

MbD::DispCompIecJecIe::DispCompIecJecIe()
{
}

MbD::DispCompIecJecIe::DispCompIecJecIe(EndFrmcptr frmi, EndFrmcptr frmj, int axis) : KinematicIeJe(frmi, frmj), axis(axis)
{
}

void MbD::DispCompIecJecIe::calc_value()
{
	aAjOIe = frmI->aAjOe(axis);
	rIeJeO = frmJ->rOeO->minusFullColumn(frmI->rOeO);
	riIeJeIe = aAjOIe->dot(rIeJeO);
}

void MbD::DispCompIecJecIe::calcPostDynCorrectorIteration()
{
	calc_value();
}

double MbD::DispCompIecJecIe::value()
{
	return riIeJeIe;
}
