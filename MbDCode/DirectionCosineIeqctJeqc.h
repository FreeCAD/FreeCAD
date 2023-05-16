#pragma once

#include "DirectionCosineIeqcJeqc.h"

namespace MbD {
    class DirectionCosineIeqctJeqc : public DirectionCosineIeqcJeqc
    {
        //pAijIeJept ppAijIeJepEIpt ppAijIeJepEJpt ppAijIeJeptpt 
    public:
        DirectionCosineIeqctJeqc();
        void initialize();

        double pAijIeJept;
        FRowDsptr ppAijIeJepEIpt;
        FRowDsptr ppAijIeJepEJpt;
        double ppAijIeJeptpt;
    };
}

