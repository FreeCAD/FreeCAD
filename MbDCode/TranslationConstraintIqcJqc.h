#pragma once

#include "TranslationConstraintIqcJc.h"

namespace MbD {
    class TranslationConstraintIqcJqc : public TranslationConstraintIqcJc
    {
        //pGpXJ pGpEJ ppGpEIpXJ ppGpEIpEJ ppGpEJpEJ iqXJ iqEJ 
    public:
        TranslationConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initriIeJeIe() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpXJ;
        FRowDsptr pGpEJ;
        FMatDsptr ppGpEIpXJ;
        FMatDsptr ppGpEIpEJ;
        FMatDsptr ppGpEJpEJ;
        int iqXJ, iqEJ;
    };
}

