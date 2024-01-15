/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIecJecIe.h"

namespace MbD {
    class DispCompIeqcJecIe : public DispCompIecJecIe
    {
        //priIeJeIepXI priIeJeIepEI ppriIeJeIepXIpEI ppriIeJeIepEIpEI pAjOIepEIT ppAjOIepEIpEI 
    public:
        DispCompIeqcJecIe();
        DispCompIeqcJecIe(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis);

        void calc_ppvaluepEIpEI() override;
        void calc_ppvaluepXIpEI() override;
        void calc_pvaluepEI() override;
        void calc_pvaluepXI() override;
        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEIpEI() override;
        FMatDsptr ppvaluepXIpEI() override;
        FRowDsptr pvaluepEI() override;
        FRowDsptr pvaluepXI() override;

        FRowDsptr priIeJeIepXI, priIeJeIepEI;
        FMatDsptr ppriIeJeIepXIpEI, ppriIeJeIepEIpEI, pAjOIepEIT;
        FMatFColDsptr ppAjOIepEIpEI;
    };
}

