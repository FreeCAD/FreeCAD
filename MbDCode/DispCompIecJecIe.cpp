#include "DispCompIecJecIe.h"

using namespace MbD;

MbD::DispCompIecJecIe::DispCompIecJecIe()
{
}

MbD::DispCompIecJecIe::DispCompIecJecIe(EndFrmcptr frmi, EndFrmcptr frmj, int axis) : KinematicIeJe(frmi, frmj), axis(axis)
{
}
