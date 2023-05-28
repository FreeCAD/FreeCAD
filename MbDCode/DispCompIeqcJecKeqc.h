#pragma once

#include "DispCompIecJecKeqc.h"

namespace MbD {
    class DispCompIeqcJecKeqc : public DispCompIecJecKeqc
    {
        //priIeJeKepXI priIeJeKepEI ppriIeJeKepXIpEK ppriIeJeKepEIpEI ppriIeJeKepEIpEK 
    public:
        DispCompIeqcJecKeqc();
        DispCompIeqcJecKeqc(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk);
        void initialize() override;
        void calcPostDynCorrectorIteration() override;

        FRowDsptr pvaluepXI();
        FRowDsptr pvaluepEI();
        FRowDsptr pvaluepEK();
        FMatDsptr ppvaluepXIpEK();
        FMatDsptr ppvaluepEIpEK();
        FMatDsptr ppvaluepEIpEI();
        FRowDsptr priIeJeKepXI;
        FRowDsptr priIeJeKepEI;
        FMatDsptr ppriIeJeKepXIpEK;
        FMatDsptr ppriIeJeKepEIpEI;
        FMatDsptr ppriIeJeKepEIpEK;
    };
}

