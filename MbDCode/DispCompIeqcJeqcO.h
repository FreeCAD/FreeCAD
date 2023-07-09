#pragma once

#include "DispCompIeqcJecO.h"

namespace MbD {
    class DispCompIeqcJeqcO : public DispCompIeqcJecO
    {
        //priIeJeOpXJ priIeJeOpEJ ppriIeJeOpEJpEJ 
    public:
        DispCompIeqcJeqcO();
        DispCompIeqcJeqcO(EndFrmcptr frmi, EndFrmcptr frmj, int axis);

        void calcPostDynCorrectorIteration() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEJpEJ() override;
        FRowDsptr pvaluepEJ() override;
        FRowDsptr pvaluepXJ() override;

        FRowDsptr priIeJeOpXJ;
        FRowDsptr priIeJeOpEJ;
        FMatDsptr ppriIeJeOpEJpEJ;
    };
}

