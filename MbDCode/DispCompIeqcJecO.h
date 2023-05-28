#pragma once

#include "DispCompIecJecO.h"

namespace MbD {
    class DispCompIeqcJecO : public DispCompIecJecO
    {
        //priIeJeOpXI priIeJeOpEI ppriIeJeOpEIpEI 
    public:

        FRowDsptr priIeJeOpXI;
        FRowDsptr priIeJeOpEI;
        FMatDsptr ppriIeJeOpEIpEI;
    };
}

