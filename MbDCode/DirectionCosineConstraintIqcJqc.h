#pragma once

#include "DirectionCosineConstraintIqcJc.h"

namespace MbD {
    class DirectionCosineConstraintIqcJqc : public DirectionCosineConstraintIqcJc
    {
        //pGpEJ ppGpEIpEJ ppGpEJpEJ iqEJ 
    public:
        DirectionCosineConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj);
        void initaAijIeJe() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpEJ;
        FMatDsptr ppGpEIpEJ;
        FMatDsptr ppGpEJpEJ;
        size_t iqEJ = -1;
    };
}

