#pragma once

#include "DispCompIecJecO.h"

namespace MbD {
    class DispCompIeqcJecO : public DispCompIecJecO
    {
        //priIeJeOpXI priIeJeOpEI ppriIeJeOpEIpEI 
    public:
        DispCompIeqcJecO();
        DispCompIeqcJecO(EndFrmcptr frmi, EndFrmcptr frmj, int axis);
        void initializeGlobally() override;
        void calcPostDynCorrectorIteration() override;

        FRowDsptr priIeJeOpXI;
        FRowDsptr priIeJeOpEI;
        FMatDsptr ppriIeJeOpEIpEI;
    };
}

