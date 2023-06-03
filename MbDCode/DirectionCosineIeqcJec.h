#pragma once

#include "DirectionCosineIecJec.h"

namespace MbD {
    class DirectionCosineIeqcJec : public DirectionCosineIecJec
    {
        //pAijIeJepEI ppAijIeJepEIpEI pAjOIepEIT ppAjOIepEIpEI 
    public:
        DirectionCosineIeqcJec();
        DirectionCosineIeqcJec(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi, size_t axisj);
        void initialize();
        void initializeGlobally();
        void calcPostDynCorrectorIteration() override;

        FRowDsptr pAijIeJepEI;
        FMatDsptr ppAijIeJepEIpEI;
        FMatDsptr pAjOIepEIT;
        std::shared_ptr<FullMatrix<std::shared_ptr<FullColumn<double>>>> ppAjOIepEIpEI;
    };
}

