#pragma once

#include "DispCompIecJecKec.h"

namespace MbD {
    class DispCompIecJecKeqc : public DispCompIecJecKec
    {
        //priIeJeKepEK ppriIeJeKepEKpEK pAjOKepEKT ppAjOKepEKpEK 
    public:
        DispCompIecJecKeqc();
        DispCompIecJecKeqc(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk);
        void initialize() override;
        void initializeGlobally() override;
        void calcPostDynCorrectorIteration() override;

        FMatDsptr ppvaluepEKpEK();
        FRowDsptr priIeJeKepEK;
        FMatDsptr ppriIeJeKepEKpEK;
        FMatDsptr pAjOKepEKT;
        FMatFColDsptr ppAjOKepEKpEK;
    };
}

