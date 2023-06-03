#pragma once

#include "DispCompIeqcJecO.h"

namespace MbD {
    class DispCompIeqcJeqcO : public DispCompIeqcJecO
    {
        //priIeJeOpXJ priIeJeOpEJ ppriIeJeOpEJpEJ 
    public:
        DispCompIeqcJeqcO();
        DispCompIeqcJeqcO(EndFrmcptr frmi, EndFrmcptr frmj, size_t axis);
        void initializeGlobally() override;
        void calcPostDynCorrectorIteration() override;

        FRowDsptr priIeJeOpXJ;
        FRowDsptr priIeJeOpEJ;
        FMatDsptr ppriIeJeOpEJpEJ;
    };
}

