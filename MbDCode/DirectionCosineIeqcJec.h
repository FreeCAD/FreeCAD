#pragma once

#include "DirectionCosineIecJec.h"

namespace MbD {
    class DirectionCosineIeqcJec : public DirectionCosineIecJec
    {
        //pAijIeJepEI ppAijIeJepEIpEI pAjOIepEIT ppAjOIepEIpEI 
    public:
        DirectionCosineIeqcJec();
        DirectionCosineIeqcJec(EndFrmsptr frmi, EndFrmsptr frmj, int axisi, int axisj);

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

