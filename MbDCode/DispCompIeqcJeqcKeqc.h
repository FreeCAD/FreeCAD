#pragma once

#include "DispCompIeqcJecKeqc.h"

namespace MbD {
    class DispCompIeqcJeqcKeqc : public DispCompIeqcJecKeqc
    {
        //priIeJeKepXJ priIeJeKepEJ ppriIeJeKepXJpEK ppriIeJeKepEJpEJ ppriIeJeKepEJpEK 
    public:

        FRowDsptr priIeJeKepXJ;
        FRowDsptr priIeJeKepEJ;
        FMatDsptr ppriIeJeKepXJpEK;
        FMatDsptr ppriIeJeKepEJpEJ;
        FMatDsptr ppriIeJeKepEJpEK;
    };
}

