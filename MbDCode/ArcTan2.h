#pragma once

#include "FunctionXY.h"

namespace MbD {
    class ArcTan2 : public FunctionXY
    {
        //
    public:
        ArcTan2() = default;
        ArcTan2(Symsptr arg, Symsptr arg1);
        double getValue() override;


    };
}

