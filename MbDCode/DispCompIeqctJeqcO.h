#pragma once

#include "DispCompIeqcJeqcO.h"

namespace MbD {
    class DispCompIeqctJeqcO : public DispCompIeqcJeqcO
    {
        //priIeJeOpt ppriIeJeOpEIpt ppriIeJeOptpt 
    public:
        DispCompIeqctJeqcO();
        DispCompIeqctJeqcO(EndFrmsptr frmi, EndFrmsptr frmj, int axis);

        void calcPostDynCorrectorIteration() override;
        void initializeGlobally() override;
        FRowDsptr ppvaluepEIpt() override;
        double ppvalueptpt() override;
        void preAccIC() override;
        void preVelIC() override;
        double pvaluept();

        double priIeJeOpt;
        FRowDsptr ppriIeJeOpEIpt;
        double ppriIeJeOptpt;
    };
}

