#pragma once

#include "DispCompIecJecKeqc.h"

namespace MbD {
    class DispCompIeqcJecKeqc : public DispCompIecJecKeqc
    {
        //priIeJeKepXI priIeJeKepEI ppriIeJeKepXIpEK ppriIeJeKepEIpEI ppriIeJeKepEIpEK 
    public:
        DispCompIeqcJecKeqc();
        DispCompIeqcJecKeqc(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, int axisk);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        FRowDsptr pvaluepXI() override;
        FRowDsptr pvaluepEI() override;
        FMatDsptr ppvaluepXIpEK() override;
        FMatDsptr ppvaluepEIpEK() override;
        FMatDsptr ppvaluepEIpEI() override;

        FRowDsptr priIeJeKepXI;
        FRowDsptr priIeJeKepEI;
        FMatDsptr ppriIeJeKepXIpEK;
        FMatDsptr ppriIeJeKepEIpEI;
        FMatDsptr ppriIeJeKepEIpEK;
    };
}

