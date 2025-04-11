/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "KinematicIeJe.h"
#include "EndFramec.h"

using namespace MbD;

KinematicIeJe::KinematicIeJe()
{
}

KinematicIeJe::KinematicIeJe(EndFrmsptr frmi, EndFrmsptr frmj) : ItemIJ(frmi, frmj)
{
}

bool MbD::KinematicIeJe::isKineIJ()
{
    return true;
}

void MbD::KinematicIeJe::calc_pvaluepXI()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_pvaluepEI()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXIpXI()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXIpEI()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEIpEI()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_pvaluepXJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_pvaluepEJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXIpXJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXIpEJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEIpXJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEIpEJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXJpXJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXJpEJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEJpEJ()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_pvaluepXK()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_pvaluepEK()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXIpEK()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEIpEK()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXJpEK()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEJpEK()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEKpEK()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_pvaluept()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvalueptpt()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_value()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXIpt()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEIpt()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXJpt()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEJpt()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepXKpt()
{
    assert(false);
}

void MbD::KinematicIeJe::calc_ppvaluepEKpt()
{
    assert(false);
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
