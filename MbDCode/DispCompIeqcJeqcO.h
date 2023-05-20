#pragma once

#include "DispCompIeqcJecO.h"

namespace MbD {
    class DispCompIeqcJeqcO : public DispCompIeqcJecO
    {
        //priIeJeOpXJ priIeJeOpEJ ppriIeJeOpEJpEJ 
    public:

        FRowDsptr priIeJeOpXJ;
        FRowDsptr priIeJeOpEJ;
        FMatDsptr ppriIeJeOpEJpEJ;
    };
}

