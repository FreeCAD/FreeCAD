#include "DispCompIecJecKec.h"

using namespace MbD;

DispCompIecJecKec::DispCompIecJecKec()
{
}

DispCompIecJecKec::DispCompIecJecKec(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, int axisk): KinematicIeJe(frmi, frmj), efrmK(frmk), axisK(axisk)
{
}

double DispCompIecJecKec::value()
{
    return riIeJeKe;
}
