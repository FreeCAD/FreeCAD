#pragma once

#include "TranslationConstraintIqcJc.h"

namespace MbD {
    class TranslationConstraintIqcJqc : public TranslationConstraintIqcJc
    {
        //pGpXJ pGpEJ ppGpEIpXJ ppGpEIpEJ ppGpEJpEJ iqXJ iqEJ 
    public:
        TranslationConstraintIqcJqc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi);
        void initriIeJeIe() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpXJ;
        FRowDsptr pGpEJ;
        FMatDsptr ppGpEIpXJ;
        FMatDsptr ppGpEIpEJ;
        FMatDsptr ppGpEJpEJ;
        size_t iqXJ = -1, iqEJ = -1;
    };
}

