#pragma once

#include "TranslationConstraintIJ.h"

namespace MbD {
    class TranslationConstraintIqcJc : public TranslationConstraintIJ
    {
        //pGpXI pGpEI ppGpXIpEI ppGpEIpEI iqXI iqEI 
    public:
        TranslationConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisk);
        void initialize();
        void initriIeJeIe() override;

        FRowDsptr pGpXI;
        FRowDsptr pGpEI;
        FMatDsptr ppGpXIpEI;
        FMatDsptr ppGpEIpEI;
        int iqXI, iqEI;
    };
}

