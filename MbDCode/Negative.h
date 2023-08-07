#pragma once

#include "FunctionX.h"

namespace MbD {
    class Negative : public FunctionX
    {
        //
    public:
        Negative() = default;
        Negative(Symsptr arg);
        double getValue() override;
        Symsptr differentiateWRTx() override;

        std::ostream& printOn(std::ostream& s) const override;

    };
}

