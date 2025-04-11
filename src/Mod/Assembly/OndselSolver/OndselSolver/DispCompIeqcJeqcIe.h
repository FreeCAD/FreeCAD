/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIeqcJecIe.h"

namespace MbD {
    class DispCompIeqcJeqcIe : public DispCompIeqcJecIe
    {
        //priIeJeIepXJ priIeJeIepEJ ppriIeJeIepEIpXJ ppriIeJeIepEIpEJ ppriIeJeIepEJpEJ 
    public:
        DispCompIeqcJeqcIe();
        DispCompIeqcJeqcIe(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis);

        void calc_ppvaluepEIpEJ() override;
        void calc_ppvaluepEIpXJ() override;
        void calc_ppvaluepEJpEJ() override;
        void calc_pvaluepEJ() override;
        void calc_pvaluepXJ() override;
        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        FMatDsptr ppvaluepEIpEJ() override;
        FMatDsptr ppvaluepEIpXJ() override;
        FMatDsptr ppvaluepEJpEJ() override;
        FRowDsptr pvaluepEJ() override;
        FRowDsptr pvaluepXJ() override;

        FRowDsptr priIeJeIepXJ, priIeJeIepEJ;
        FMatDsptr ppriIeJeIepEIpXJ, ppriIeJeIepEIpEJ, ppriIeJeIepEJpEJ;
    };
}

