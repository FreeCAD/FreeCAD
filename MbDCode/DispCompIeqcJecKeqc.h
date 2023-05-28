#pragma once

#include "DispCompIecJecKeqc.h"

namespace MbD {
    class DispCompIeqcJecKeqc : public DispCompIecJecKeqc
    {
        //priIeJeKepXI priIeJeKepEI ppriIeJeKepXIpEK ppriIeJeKepEIpEI ppriIeJeKepEIpEK 
    public:

        FRowDsptr priIeJeKepXI;
        FRowDsptr priIeJeKepEI;
        FMatDsptr ppriIeJeKepXIpEK;
        FMatDsptr ppriIeJeKepEIpEI;
        FMatDsptr ppriIeJeKepEIpEK;
    };
}

