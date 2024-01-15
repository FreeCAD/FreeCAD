/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "DirectionCosineIecJec.h"

namespace MbD {
    class DirectionCosineIeqcJec : public DirectionCosineIecJec
    {
        //pAijIeJepEI ppAijIeJepEIpEI pAjOIepEIT ppAjOIepEIpEI 
    public:
        DirectionCosineIeqcJec();
        DirectionCosineIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj, size_t axisi, size_t axisj);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEIpEI() override;
        FRowDsptr pvaluepEI() override;

        FRowDsptr pAijIeJepEI;
        FMatDsptr ppAijIeJepEIpEI;
        FMatDsptr pAjOIepEIT;
        FMatFColDsptr ppAjOIepEIpEI;
    };
}

