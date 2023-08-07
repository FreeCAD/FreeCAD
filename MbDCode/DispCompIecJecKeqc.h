#pragma once

#include "DispCompIecJecKec.h"

namespace MbD {
    class DispCompIecJecKeqc : public DispCompIecJecKec
    {
        //priIeJeKepEK ppriIeJeKepEKpEK pAjOKepEKT ppAjOKepEKpEK 
    public:
        DispCompIecJecKeqc();
        DispCompIecJecKeqc(EndFrmsptr frmi, EndFrmsptr frmj, EndFrmsptr frmk, int axisk);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FMatDsptr ppvaluepEKpEK() override;
        FRowDsptr pvaluepEK() override;

        FRowDsptr priIeJeKepEK;
        FMatDsptr ppriIeJeKepEKpEK;
        FMatDsptr pAjOKepEKT;
        FMatFColDsptr ppAjOKepEKpEK;
    };
}

