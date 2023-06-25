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
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void fillAccICIterError(FColDsptr col) override;

        FRowDsptr pGpEJ;
        FMatDsptr ppGpEIpEJ;
        FMatDsptr ppGpEJpEJ;
        int iqEJ = -1;
    };
}

