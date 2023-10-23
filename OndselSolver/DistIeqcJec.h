/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DistIecJec.h"

namespace MbD {
    class DistIeqcJec : public DistIecJec
    {
        //prIeJepXI prIeJepEI pprIeJepXIpXI pprIeJepXIpEI pprIeJepEIpEI mprIeJeOpEIT 
    public:
        DistIeqcJec();
        DistIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj);

        void calcPrivate() override;
        void initialize() override;
        FMatDsptr ppvaluepEIpEI() override;
        FMatDsptr ppvaluepXIpEI() override;
        FMatDsptr ppvaluepXIpXI() override;
        FRowDsptr pvaluepEI() override;
        FRowDsptr pvaluepXI() override;

        FRowDsptr prIeJepXI, prIeJepEI;
        FMatDsptr pprIeJepXIpXI, pprIeJepXIpEI, pprIeJepEIpEI, mprIeJeOpEIT;
    };
}

