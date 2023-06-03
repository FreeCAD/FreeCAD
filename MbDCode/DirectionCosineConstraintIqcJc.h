#pragma once

#include "DirectionCosineConstraintIJ.h"

namespace MbD {
    class DirectionCosineConstraintIqcJc : public DirectionCosineConstraintIJ
    {
        //pGpEI ppGpEIpEI iqEI 
    public:
        DirectionCosineConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj);
        void initaAijIeJe() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpEI;
        FMatDsptr ppGpEIpEI;
        size_t iqEI = -1;
    };
}

