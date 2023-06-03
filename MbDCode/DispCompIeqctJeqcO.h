#pragma once

#include "DispCompIeqcJeqcO.h"

namespace MbD {
    class DispCompIeqctJeqcO : public DispCompIeqcJeqcO
    {
        //priIeJeOpt ppriIeJeOpEIpt ppriIeJeOptpt 
    public:
        DispCompIeqctJeqcO();
        DispCompIeqctJeqcO(EndFrmcptr frmi, EndFrmcptr frmj, size_t axis);
        void initializeGlobally() override;
        void calcPostDynCorrectorIteration() override;

        double priIeJeOpt;
        FRowDsptr ppriIeJeOpEIpt;
        double ppriIeJeOptpt;
    };
}

