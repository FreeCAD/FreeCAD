#pragma once

#include "DispCompIeqcJecO.h"

namespace MbD {
    class DispCompIeqcJeqcO : public DispCompIeqcJecO
    {
        //priIeJeOpXJ priIeJeOpEJ ppriIeJeOpEJpEJ 
    public:
        DispCompIeqcJeqcO();
        DispCompIeqcJeqcO(EndFrmcptr frmi, EndFrmcptr frmj, int axis);
        void initializeGlobally() override;
        void calcPostDynCorrectorIteration() override;

        FRowDsptr priIeJeOpXJ;
        FRowDsptr priIeJeOpEJ;
        FMatDsptr ppriIeJeOpEJpEJ;
    };
}

