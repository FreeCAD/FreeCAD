#pragma once

#include "TranslationConstraintIqcJqc.h"

namespace MbD {
    class TranslationConstraintIqctJqc : public TranslationConstraintIqcJqc
    {
        //pGpt ppGpXIpt ppGpEIpt ppGpXJpt ppGpEJpt ppGptpt 
    public:
        TranslationConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);

        void fillAccICIterError(FColDsptr col) override;
        void fillVelICError(FColDsptr col) override;
        void initriIeJeIe() override;
        void preAccIC() override;
        void preVelIC() override;
        ConstraintType type() override;

        double pGpt;
        FRowDsptr ppGpXIpt;
        FRowDsptr ppGpEIpt;
        FRowDsptr ppGpXJpt;
        FRowDsptr ppGpEJpt;
        double ppGptpt;
    };
}

