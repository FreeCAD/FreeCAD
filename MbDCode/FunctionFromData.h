#pragma once

#include "FunctionXcParameter.h"

namespace MbD {
    class FunctionFromData : public FunctionXcParameter
    {
        //xvalue xs ys 
    public:
        FunctionFromData() = default;
        FunctionFromData(Symsptr arg);

        double xvalue;
        std::shared_ptr<std::vector<double>> xs, ys;
    };
}

