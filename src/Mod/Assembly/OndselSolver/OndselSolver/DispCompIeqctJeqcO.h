/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DispCompIeqcJeqcO.h"

namespace MbD {
    class DispCompIeqctJeqcO : public DispCompIeqcJeqcO
    {
        //priIeJeOpt ppriIeJeOpEIpt ppriIeJeOptpt 
    public:
        DispCompIeqctJeqcO();
        DispCompIeqctJeqcO(EndFrmsptr frmi, EndFrmsptr frmj, size_t axis);

        void calcPostDynCorrectorIteration() override;
        void initializeGlobally() override;
        FRowDsptr ppvaluepEIpt() override;
        double ppvalueptpt() override;
        void preAccIC() override;
        void preVelIC() override;
        double pvaluept() override;

        double priIeJeOpt;
        FRowDsptr ppriIeJeOpEIpt;
        double ppriIeJeOptpt;
    };
}

