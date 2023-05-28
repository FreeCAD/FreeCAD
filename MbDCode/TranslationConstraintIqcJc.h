#pragma once

#include "TranslationConstraintIJ.h"

namespace MbD {
    class TranslationConstraintIqcJc : public TranslationConstraintIJ
    {
        //pGpXI pGpEI ppGpXIpEI ppGpEIpEI iqXI iqEI 
    public:
        TranslationConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initriIeJeIe() override;
        void calcPostDynCorrectorIteration() override;
        void useEquationNumbers() override;

        FRowDsptr pGpXI;
        FRowDsptr pGpEI;
        FMatDsptr ppGpXIpEI;
        FMatDsptr ppGpEIpEI;
        int iqXI, iqEI;
    };
}

