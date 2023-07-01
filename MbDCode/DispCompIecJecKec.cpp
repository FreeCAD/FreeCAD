#include "DispCompIecJecKec.h"

using namespace MbD;

DispCompIecJecKec::DispCompIecJecKec()
{
}

DispCompIecJecKec::DispCompIecJecKec(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk): KinematicIeJe(frmi, frmj), efrmK(frmk), axisK(axisk)
{
}

FRowDsptr DispCompIecJecKec::pvaluepXJ()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr DispCompIecJecKec::pvaluepEJ()
{
    assert(false);
    return FRowDsptr();
}

FMatDsptr DispCompIecJecKec::ppvaluepXJpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr DispCompIecJecKec::ppvaluepEJpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr DispCompIecJecKec::ppvaluepEJpEJ()
{
    assert(false);
    return FMatDsptr();
}

double DispCompIecJecKec::value()
{
    return riIeJeKe;
}
