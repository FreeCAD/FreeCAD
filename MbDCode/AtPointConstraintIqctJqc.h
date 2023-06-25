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
        MbD::ConstraintType type() override;
        void preVelIC() override;
        void fillVelICError(FColDsptr col) override;
        void fillAccICIterError(FColDsptr col) override;

        double pGpt;
        FRowDsptr ppGpEIpt;
        double ppGptpt;
        void preAccIC() override;


    };
}

