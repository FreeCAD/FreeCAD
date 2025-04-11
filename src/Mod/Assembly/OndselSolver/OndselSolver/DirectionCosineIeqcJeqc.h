/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DirectionCosineIeqcJec.h"

namespace MbD {
    class DirectionCosineIeqcJeqc : public DirectionCosineIeqcJec
    {
        //pAijIeJepEJ ppAijIeJepEIpEJ ppAijIeJepEJpEJ pAjOJepEJT ppAjOJepEJpEJ 
    public:
        DirectionCosineIeqcJeqc();
        DirectionCosineIeqcJeqc(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEIpEJ() override;
        FMatDsptr ppvaluepEJpEJ() override;
        FRowDsptr pvaluepEJ() override;

        FRowDsptr pAijIeJepEJ;
        FMatDsptr ppAijIeJepEIpEJ;
        FMatDsptr ppAijIeJepEJpEJ;
        FMatDsptr pAjOJepEJT;
        FMatFColDsptr ppAjOJepEJpEJ;

    };
}

