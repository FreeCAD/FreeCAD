#include "DispCompIecJecKec.h"

using namespace MbD;

MbD::DispCompIecJecKec::DispCompIecJecKec()
{
}

MbD::DispCompIecJecKec::DispCompIecJecKec(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk): KinematicIeJe(frmi, frmj), efrmK(frmk), axisK(axisk)
{
}

FRowDsptr MbD::DispCompIecJecKec::pvaluepXJ()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::DispCompIecJecKec::pvaluepEJ()
{
    assert(false);
    return FRowDsptr();
}

FMatDsptr MbD::DispCompIecJecKec::ppvaluepXJpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::DispCompIecJecKec::ppvaluepEJpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::DispCompIecJecKec::ppvaluepEJpEJ()
{
    assert(false);
    return FMatDsptr();
}

double MbD::DispCompIecJecKec::value()
{
    return riIeJeKe;
}
