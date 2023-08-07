#pragma once

#include "DirectionCosineConstraintIqcJc.h"

namespace MbD {
    class DirectionCosineConstraintIqcJqc : public DirectionCosineConstraintIqcJc
    {
        //pGpEJ ppGpEIpEJ ppGpEJpEJ iqEJ 
    public:
        DirectionCosineConstraintIqcJqc(EndFrmsptr frmi, EndFrmsptr frmj, int axisi, int axisj);

        void calcPostDynCorrectorIteration() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void initaAijIeJe() override;
        void useEquationNumbers() override;

        FRowDsptr pGpEJ;
        FMatDsptr ppGpEIpEJ;
        FMatDsptr ppGpEJpEJ;
        int iqEJ = -1;
    };
}

