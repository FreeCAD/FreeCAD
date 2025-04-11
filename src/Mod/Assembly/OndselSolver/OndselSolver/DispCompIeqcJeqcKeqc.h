/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIeqcJecKeqc.h"

namespace MbD {
    class DispCompIeqcJeqcKeqc : public DispCompIeqcJecKeqc
    {
        //priIeJeKepXJ priIeJeKepEJ ppriIeJeKepXJpEK ppriIeJeKepEJpEJ ppriIeJeKepEJpEK 
    public:
        DispCompIeqcJeqcKeqc();
        DispCompIeqcJeqcKeqc(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, size_t axisk);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        FRowDsptr pvaluepXJ() override;
        FRowDsptr pvaluepEJ() override;
        FMatDsptr ppvaluepXJpEK() override;
        FMatDsptr ppvaluepEJpEK() override;
        FMatDsptr ppvaluepEJpEJ() override;

        FRowDsptr priIeJeKepXJ;
        FRowDsptr priIeJeKepEJ;
        FMatDsptr ppriIeJeKepXJpEK;
        FMatDsptr ppriIeJeKepEJpEJ;
        FMatDsptr ppriIeJeKepEJpEK;
    };
}

