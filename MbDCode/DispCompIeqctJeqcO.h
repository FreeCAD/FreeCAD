#pragma once

#include "DispCompIeqcJeqcO.h"

namespace MbD {
    class DispCompIeqctJeqcO : public DispCompIeqcJeqcO
    {
        //priIeJeOpt ppriIeJeOpEIpt ppriIeJeOptpt 
    public:
        DispCompIeqctJeqcO();
        DispCompIeqctJeqcO(EndFrmcptr frmi, EndFrmcptr frmj, int axis);
        void initializeGlobally() override;
        void calcPostDynCorrectorIteration() override;
        void preVelIC() override;
        double pvaluept();
        void preAccIC() override;

        double priIeJeOpt;
        FRowDsptr ppriIeJeOpEIpt;
        double ppriIeJeOptpt;
    };
}

