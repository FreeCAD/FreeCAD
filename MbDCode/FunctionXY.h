#pragma once

#include "Function.h"

namespace MbD {
    class FunctionXY : public Function
    {
        //x y 
    public:
        FunctionXY();
        FunctionXY(Symsptr base, Symsptr exp);

        Symsptr x, y;

    };
}

