#pragma once

#include "DirectionCosineConstraintIqcJc.h"

namespace MbD {
    class DirectionCosineConstraintIqcJqc : public DirectionCosineConstraintIqcJc
    {
        //pGpEJ ppGpEIpEJ ppGpEJpEJ iqEJ 
    public:
        DirectionCosineConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);
        void initaAijIeJe() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpEJ;
        FMatDsptr ppGpEIpEJ;
        FMatDsptr ppGpEJpEJ;
        int iqEJ;
    };
}

