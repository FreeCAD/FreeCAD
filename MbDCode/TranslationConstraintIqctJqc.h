#pragma once

#include "TranslationConstraintIqcJqc.h"

namespace MbD {
    class TranslationConstraintIqctJqc : public TranslationConstraintIqcJqc
    {
        //pGpt ppGpXIpt ppGpEIpt ppGpXJpt ppGpEJpt ppGptpt 
    public:
        TranslationConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initriIeJeIe() override;

        double pGpt;
        FRowDsptr ppGpXIpt;
        FRowDsptr ppGpEIpt;
        FRowDsptr ppGpXJpt;
        FRowDsptr ppGpEJpt;
        double ppGptpt;
    };
}

