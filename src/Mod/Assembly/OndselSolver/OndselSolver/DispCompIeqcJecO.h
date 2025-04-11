/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIecJecO.h"

namespace MbD {
    class DispCompIeqcJecO : public DispCompIecJecO
    {
        //priIeJeOpXI priIeJeOpEI ppriIeJeOpEIpEI 
    public:
        DispCompIeqcJecO();
        DispCompIeqcJecO(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis);

        void calcPostDynCorrectorIteration() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEIpEI() override;
        FRowDsptr pvaluepEI() override;
        FRowDsptr pvaluepXI() override;

        FRowDsptr priIeJeOpXI;
        FRowDsptr priIeJeOpEI;
        FMatDsptr ppriIeJeOpEIpEI;
    };
}

