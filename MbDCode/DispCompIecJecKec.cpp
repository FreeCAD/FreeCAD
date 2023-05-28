#include "DispCompIecJecKec.h"

MbD::DispCompIecJecKec::DispCompIecJecKec()
{
}

MbD::DispCompIecJecKec::DispCompIecJecKec(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk): KinematicIeJe(frmi, frmj), efrmK(frmk), axisK(axisk)
{
}

double MbD::DispCompIecJecKec::value()
{
    return riIeJeKe;
}
