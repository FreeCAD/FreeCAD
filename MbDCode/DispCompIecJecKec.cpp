#include "DispCompIecJecKec.h"

using namespace MbD;

MbD::DispCompIecJecKec::DispCompIecJecKec()
{
}

MbD::DispCompIecJecKec::DispCompIecJecKec(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, size_t axisk): KinematicIeJe(frmi, frmj), efrmK(frmk), axisK(axisk)
{
}

double MbD::DispCompIecJecKec::value()
{
    return riIeJeKe;
}
