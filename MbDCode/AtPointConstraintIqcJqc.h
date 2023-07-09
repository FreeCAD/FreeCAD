#pragma once

#include "AtPointConstraintIqcJc.h"

namespace MbD {
    class AtPointConstraintIqcJqc : public AtPointConstraintIqcJc
    {
        //pGpEJ ppGpEJpEJ iqXJminusOnePlusAxis iqEJ 
    public:
        AtPointConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);

        void calcPostDynCorrectorIteration() override;
        void initializeGlobally() override;
        void initriIeJeO() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void fillVelICJacob(SpMatDsptr mat) override;
        void useEquationNumbers() override;

        FRowDsptr pGpEJ;
        FMatDsptr ppGpEJpEJ;
        int iqXJminusOnePlusAxis = -1, iqEJ = -1;
    };
}

