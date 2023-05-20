#pragma once

#include "DispCompIeqcJeqcO.h"

namespace MbD {
    class DispCompIeqctJeqcO : public DispCompIeqcJeqcO
    {
        //priIeJeOpt ppriIeJeOpEIpt ppriIeJeOptpt 
    public:

        double priIeJeOpt;
        FRowDsptr ppriIeJeOpEIpt;
        double ppriIeJeOptpt;
    };
}

