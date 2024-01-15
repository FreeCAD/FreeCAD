/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIeqcJecO.h"

namespace MbD {
    class DispCompIeqcJeqcO : public DispCompIeqcJecO
    {
        //priIeJeOpXJ priIeJeOpEJ ppriIeJeOpEJpEJ 
    public:
        DispCompIeqcJeqcO();
        DispCompIeqcJeqcO(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis);

        void calcPostDynCorrectorIteration() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEJpEJ() override;
        FRowDsptr pvaluepEJ() override;
        FRowDsptr pvaluepXJ() override;

        FRowDsptr priIeJeOpXJ;
        FRowDsptr priIeJeOpEJ;
        FMatDsptr ppriIeJeOpEJpEJ;
    };
}

