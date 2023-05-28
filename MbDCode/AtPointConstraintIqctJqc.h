#pragma once

#include "AtPointConstraintIqcJqc.h"

namespace MbD {
    class AtPointConstraintIqctJqc : public AtPointConstraintIqcJqc
    {
        //pGpt ppGpEIpt ppGptpt 
    public:
        AtPointConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initializeGlobally() override;
        void initriIeJeO() override;
        void calcPostDynCorrectorIteration() override;

        double pGpt;
        FRowDsptr ppGpEIpt;
        double ppGptpt;
    };
}

