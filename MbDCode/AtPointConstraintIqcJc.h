#pragma once

#include "AtPointConstraintIJ.h"

namespace MbD {
    class AtPointConstraintIqcJc : public AtPointConstraintIJ
    {
        //pGpEI ppGpEIpEI iqXIminusOnePlusAxis iqEI 
    public:
        AtPointConstraintIqcJc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi);
        void initialize();
        void initriIeJeO() override;
        void calcPostDynCorrectorIteration() override;

        FRowDsptr pGpEI;
        FMatDsptr ppGpEIpEI;
        int iqXIminusOnePlusAxis, iqEI;
    };
}

