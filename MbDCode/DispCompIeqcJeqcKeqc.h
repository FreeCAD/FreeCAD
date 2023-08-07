#pragma once

#include "DispCompIeqcJecKeqc.h"

namespace MbD {
    class DispCompIeqcJeqcKeqc : public DispCompIeqcJecKeqc
    {
        //priIeJeKepXJ priIeJeKepEJ ppriIeJeKepXJpEK ppriIeJeKepEJpEJ ppriIeJeKepEJpEK 
    public:
        DispCompIeqcJeqcKeqc();
        DispCompIeqcJeqcKeqc(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, int axisk);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        FRowDsptr pvaluepXJ() override;
        FRowDsptr pvaluepEJ() override;
        FMatDsptr ppvaluepXJpEK() override;
        FMatDsptr ppvaluepEJpEK() override;
        FMatDsptr ppvaluepEJpEJ() override;

        FRowDsptr priIeJeKepXJ;
        FRowDsptr priIeJeKepEJ;
        FMatDsptr ppriIeJeKepXJpEK;
        FMatDsptr ppriIeJeKepEJpEJ;
        FMatDsptr ppriIeJeKepEJpEK;
    };
}

