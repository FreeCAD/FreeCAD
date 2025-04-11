/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIeqcJeqcKeqc.h"

namespace MbD {
    class DispCompIeqcJeqcKeqct : public DispCompIeqcJeqcKeqc
    {
        //priIeJeKept ppriIeJeKepXIpt ppriIeJeKepEIpt ppriIeJeKepXJpt ppriIeJeKepEJpt ppriIeJeKepEKpt ppriIeJeKeptpt 
    public:
        DispCompIeqcJeqcKeqct();
        DispCompIeqcJeqcKeqct(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, size_t axisk);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FRowDsptr ppvaluepXIpt() override;
        FRowDsptr ppvaluepEIpt() override;
        FRowDsptr ppvaluepEKpt() override;
        FRowDsptr ppvaluepXJpt() override;
        FRowDsptr ppvaluepEJpt() override;
        double ppvalueptpt() override;
        double pvaluept() override;
        void preAccIC() override;
        void preVelIC() override;

        double priIeJeKept;
        FRowDsptr ppriIeJeKepXIpt;
        FRowDsptr ppriIeJeKepEIpt;
        FRowDsptr ppriIeJeKepXJpt;
        FRowDsptr ppriIeJeKepEJpt;
        FRowDsptr ppriIeJeKepEKpt;
        double ppriIeJeKeptpt;
    };
}

