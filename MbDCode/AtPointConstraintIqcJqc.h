#pragma once

#include "AtPointConstraintIqcJc.h"

namespace MbD {
    class AtPointConstraintIqcJqc : public AtPointConstraintIqcJc
    {
        //pGpEJ ppGpEJpEJ iqXJminusOnePlusAxis iqEJ 
    public:
        AtPointConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initializeGlobally() override;
        void initriIeJeO() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpEJ;
        FMatDsptr ppGpEJpEJ;
        int iqXJminusOnePlusAxis, iqEJ;
    };
}

