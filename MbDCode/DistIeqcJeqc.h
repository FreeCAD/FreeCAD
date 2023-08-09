/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DistIeqcJec.h"

namespace MbD {
    class DistIeqcJeqc : public DistIeqcJec
    {
        //prIeJepXJ prIeJepEJ pprIeJepXIpXJ pprIeJepEIpXJ pprIeJepXJpXJ pprIeJepXIpEJ pprIeJepEIpEJ pprIeJepXJpEJ pprIeJepEJpEJ prIeJeOpEJT 
    public:
        DistIeqcJeqc();
        DistIeqcJeqc(EndFrmsptr frmi, EndFrmsptr frmj);
        
        void calcPrivate() override;
        void initialize() override;
        FMatDsptr ppvaluepEIpEJ();
        FMatDsptr ppvaluepEIpXJ();
        FMatDsptr ppvaluepEJpEJ();
        FMatDsptr ppvaluepXIpEJ();
        FMatDsptr ppvaluepXIpXJ();
        FMatDsptr ppvaluepXJpEJ();
        FMatDsptr ppvaluepXJpXJ();
        FRowDsptr pvaluepEJ();
        FRowDsptr pvaluepXJ();

        FRowDsptr prIeJepXJ, prIeJepEJ;
        FMatDsptr pprIeJepXIpXJ, pprIeJepEIpXJ, pprIeJepXJpXJ, pprIeJepXIpEJ, pprIeJepEIpEJ, pprIeJepXJpEJ, pprIeJepEJpEJ, prIeJeOpEJT;
    };
}

