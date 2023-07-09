#pragma once

#include "DistanceConstraintIqcJc.h"

namespace MbD {
    class DistanceConstraintIqcJqc : public DistanceConstraintIqcJc
    {
        //pGpXJ pGpEJ ppGpXIpXJ ppGpEIpXJ ppGpXJpXJ ppGpXIpEJ ppGpEIpEJ ppGpXJpEJ ppGpEJpEJ iqXJ iqEJ 
    public:
        DistanceConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj);

        void calcPostDynCorrectorIteration() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void init_distIeJe() override;
        void useEquationNumbers() override;

        FRowDsptr pGpXJ, pGpEJ;
        FMatDsptr ppGpXIpXJ, ppGpEIpXJ, ppGpXJpXJ, ppGpXIpEJ, ppGpEIpEJ, ppGpXJpEJ, ppGpEJpEJ;
        int iqXJ, iqEJ;
    };
}

