#pragma once

#include "TranslationConstraintIJ.h"

namespace MbD {
    class TranslationConstraintIqcJc : public TranslationConstraintIJ
    {
        //pGpXI pGpEI ppGpXIpEI ppGpEIpEI iqXI iqEI 
    public:
        TranslationConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi);
        void initriIeJeIe() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpXI;
        FRowDsptr pGpEI;
        FMatDsptr ppGpXIpEI;
        FMatDsptr ppGpEIpEI;
        size_t iqXI, iqEI;
    };
}

