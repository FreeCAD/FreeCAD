#include "DispCompIecJecKec.h"

using namespace MbD;

DispCompIecJecKec::DispCompIecJecKec()
{
}

DispCompIecJecKec::DispCompIecJecKec(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk): KinematicIeJe(frmi, frmj), efrmK(frmk), axisK(axisk)
{
}

double DispCompIecJecKec::value()
{
    return riIeJeKe;
}
