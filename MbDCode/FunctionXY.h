#pragma once

#include "Function.h"

namespace MbD {
    class Symbolic;
    //using Symsptr = Symsptr;

    class FunctionXY : public Function
    {
        //x y 
    public:
        FunctionXY();
        FunctionXY(Symsptr base, Symsptr exp);
        void arguments(Symsptr args) override;
        virtual Symsptr differentiateWRTx() = 0;
        virtual Symsptr differentiateWRTy() = 0;

        Symsptr x, y;

    };
}

