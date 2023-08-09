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
        DispCompIeqcJeqcKeqct(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, int axisk);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FRowDsptr ppvaluepXIpt();
        FRowDsptr ppvaluepEIpt();
        FRowDsptr ppvaluepEKpt();
        FRowDsptr ppvaluepXJpt();
        FRowDsptr ppvaluepEJpt();
        double ppvalueptpt();
        double pvaluept();
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

