#pragma once

#include "DispCompIeqcJeqcKeqc.h"

namespace MbD {
    class DispCompIeqcJeqcKeqct : public DispCompIeqcJeqcKeqc
    {
        //priIeJeKept ppriIeJeKepXIpt ppriIeJeKepEIpt ppriIeJeKepXJpt ppriIeJeKepEJpt ppriIeJeKepEKpt ppriIeJeKeptpt 
    public:
        DispCompIeqcJeqcKeqct();
        DispCompIeqcJeqcKeqct(EndFrmcptr frmi, EndFrmcptr frmj, EndFrmcptr frmk, int axisk);
        void initialize() override;
        void calcPostDynCorrectorIteration() override;
        void preVelIC() override;
        double pvaluept();
        FRowDsptr ppvaluepXIpt();
        FRowDsptr ppvaluepEIpt();
        FRowDsptr ppvaluepEKpt();
        FRowDsptr ppvaluepXJpt();
        FRowDsptr ppvaluepEJpt();
        double ppvalueptpt();
        void preAccIC() override;

        double priIeJeKept;
        FRowDsptr ppriIeJeKepXIpt;
        FRowDsptr ppriIeJeKepEIpt;
        FRowDsptr ppriIeJeKepXJpt;
        FRowDsptr ppriIeJeKepEJpt;
        FRowDsptr ppriIeJeKepEKpt;
        double ppriIeJeKeptpt;
    };
}

