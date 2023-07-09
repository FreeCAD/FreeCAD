#pragma once

#include "DirectionCosineIeqcJeqc.h"

namespace MbD {
    class DirectionCosineIeqctJeqc : public DirectionCosineIeqcJeqc
    {
        //pAijIeJept ppAijIeJepEIpt ppAijIeJepEJpt ppAijIeJeptpt 
    public:
        DirectionCosineIeqctJeqc();
        DirectionCosineIeqctJeqc(EndFrmcptr frmi, EndFrmcptr frmj, int axisi, int axisj);

        void calcPostDynCorrectorIteration() override;
        void initialize() override;
        void initializeGlobally() override;
        FRowDsptr ppvaluepEIpt() override;
        FRowDsptr ppvaluepEJpt() override;
        double ppvalueptpt() override;
        void preAccIC() override;
        void preVelIC() override;
        double pvaluept() override;

        double pAijIeJept;
        FRowDsptr ppAijIeJepEIpt;
        FRowDsptr ppAijIeJepEJpt;
        double ppAijIeJeptpt;
    };
}

