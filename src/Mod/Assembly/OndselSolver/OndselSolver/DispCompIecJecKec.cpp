/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "DispCompIecJecKec.h"

using namespace MbD;

DispCompIecJecKec::DispCompIecJecKec()
{
}

DispCompIecJecKec::DispCompIecJecKec(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, size_t axisk): KinematicIeJe(frmi, frmj), efrmK(frmk), axisK(axisk)
{
}

double DispCompIecJecKec::value()
{
    return riIeJeKe;
}
