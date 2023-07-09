#include "KinematicIeJe.h"

using namespace MbD;

KinematicIeJe::KinematicIeJe()
{
}

KinematicIeJe::KinematicIeJe(EndFrmcptr frmi, EndFrmcptr frmj) : frmI(frmi), frmJ(frmj)
{
}

bool MbD::KinematicIeJe::isKineIJ()
{
    return true;
}

FRowDsptr MbD::KinematicIeJe::pvaluepXI()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::pvaluepEI()
{
    assert(false);
    return FRowDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXIpXI()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXIpEI()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepEIpEI()
{
    assert(false);
    return FMatDsptr();
}

FRowDsptr MbD::KinematicIeJe::pvaluepXJ()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::pvaluepEJ()
{
    assert(false);
    return FRowDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXIpXJ()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXIpEJ()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepEIpXJ()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepEIpEJ()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXJpXJ()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXJpEJ()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepEJpEJ()
{
    assert(false);
    return FMatDsptr();
}

FRowDsptr MbD::KinematicIeJe::pvaluepXK()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::pvaluepEK()
{
    assert(false);
    return FRowDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXIpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepEIpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepXJpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepEJpEK()
{
    assert(false);
    return FMatDsptr();
}

FMatDsptr MbD::KinematicIeJe::ppvaluepEKpEK()
{
    assert(false);
    return FMatDsptr();
}

double MbD::KinematicIeJe::pvaluept()
{
    assert(false);
    return 0.0;
}

double MbD::KinematicIeJe::ppvalueptpt()
{
    assert(false);
    return 0.0;
}

FRowDsptr MbD::KinematicIeJe::ppvaluepXIpt()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::ppvaluepEIpt()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::ppvaluepXJpt()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::ppvaluepEJpt()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::ppvaluepXKpt()
{
    assert(false);
    return FRowDsptr();
}

FRowDsptr MbD::KinematicIeJe::ppvaluepEKpt()
{
    assert(false);
    return FRowDsptr();
}

double MbD::KinematicIeJe::value()
{
    assert(false);
    return 0.0;
}
