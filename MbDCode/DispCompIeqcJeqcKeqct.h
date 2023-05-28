#pragma once

#include "DispCompIeqcJeqcKeqc.h"

namespace MbD {
    class DispCompIeqcJeqcKeqct : public DispCompIeqcJeqcKeqc
    {
        //priIeJeKept ppriIeJeKepXIpt ppriIeJeKepEIpt ppriIeJeKepXJpt ppriIeJeKepEJpt ppriIeJeKepEKpt ppriIeJeKeptpt 
    public:

        double priIeJeKept;
        FRowDsptr ppriIeJeKepXIpt;
        FRowDsptr ppriIeJeKepEIpt;
        FRowDsptr ppriIeJeKepXJpt;
        FRowDsptr ppriIeJeKepEJpt;
        FRowDsptr ppriIeJeKepEKpt;
        double ppriIeJeKeptpt;
    };
}

