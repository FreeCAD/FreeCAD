#pragma once

#include "AtPointConstraintIqcJqc.h"

namespace MbD {
    class AtPointConstraintIqctJqc : public AtPointConstraintIqcJqc
    {
        //pGpt ppGpEIpt ppGptpt 
    public:
        AtPointConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);

        void calcPostDynCorrectorIteration() override;
        void fillAccICIterError(FColDsptr col) override;
        void fillVelICError(FColDsptr col) override;
        void initializeGlobally() override;
        void initriIeJeO() override;
        void preAccIC() override;
        void preVelIC() override;
        ConstraintType type() override;

        double pGpt;
        FRowDsptr ppGpEIpt;
        double ppGptpt;

    };
}

