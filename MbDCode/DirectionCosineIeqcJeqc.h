#pragma once

#include "DirectionCosineIeqcJec.h"

namespace MbD {
    class DirectionCosineIeqcJeqc : public DirectionCosineIeqcJec
    {
        //pAijIeJepEJ ppAijIeJepEIpEJ ppAijIeJepEJpEJ pAjOJepEJT ppAjOJepEJpEJ 
    public:
        DirectionCosineIeqcJeqc();
        DirectionCosineIeqcJeqc(EndFrmsptr frmi, EndFrmsptr frmj, int axisi, int axisj);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEIpEJ() override;
        FMatDsptr ppvaluepEJpEJ() override;
        FRowDsptr pvaluepEJ() override;

        FRowDsptr pAijIeJepEJ;
        FMatDsptr ppAijIeJepEIpEJ;
        FMatDsptr ppAijIeJepEJpEJ;
        FMatDsptr pAjOJepEJT;
        FMatFColDsptr ppAjOJepEJpEJ;

    };
}

