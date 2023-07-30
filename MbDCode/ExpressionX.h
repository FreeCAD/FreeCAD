#pragma once

#include "FunctionX.h"

namespace MbD {
    class ExpressionX : public FunctionX
    {
        //
    public:

        void xexpression(Symsptr arg, Symsptr func);
        Symsptr differentiateWRT(Symsptr var) override;
        double getValue() override;

        Symsptr expression = std::make_shared<Symbolic>();
    };
}

