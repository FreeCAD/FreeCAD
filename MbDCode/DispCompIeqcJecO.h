#pragma once

#include "DispCompIecJecO.h"

namespace MbD {
    class DispCompIeqcJecO : public DispCompIecJecO
    {
        //priIeJeOpXI priIeJeOpEI ppriIeJeOpEIpEI 
    public:
        DispCompIeqcJecO();
        DispCompIeqcJecO(EndFrmcptr frmi, EndFrmcptr frmj, int axis);

        void calcPostDynCorrectorIteration() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEIpEI() override;
        FRowDsptr pvaluepEI() override;
        FRowDsptr pvaluepXI() override;

        FRowDsptr priIeJeOpXI;
        FRowDsptr priIeJeOpEI;
        FMatDsptr ppriIeJeOpEIpEI;
    };
}

