#pragma once

#include "AngleZIeqcJec.h"

namespace MbD {
    class AngleZIeqcJeqc : public AngleZIeqcJec
    {
        //pthezpEJ ppthezpEIpEJ ppthezpEJpEJ 
    public:
        AngleZIeqcJeqc();
        AngleZIeqcJeqc(EndFrmsptr frmi, EndFrmsptr frmj);
        
        void calcPostDynCorrectorIteration() override;
        void init_aAijIeJe() override;
        void initialize() override;
        FMatDsptr ppvaluepEIpEJ() override;
        FMatDsptr ppvaluepEJpEJ() override;
        FRowDsptr pvaluepEJ() override;

        FRowDsptr pthezpEJ;
        FMatDsptr ppthezpEIpEJ, ppthezpEJpEJ;
    };
}

