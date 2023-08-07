#pragma once

#include "FunctionX.h"

namespace MbD {
    class Sine : public FunctionX
    {
        //
    public:
        Sine() = default;
        Sine(Symsptr arg);
        double getValue() override;
        Symsptr differentiateWRTx() override;

        std::ostream& printOn(std::ostream& s) const override;

    };
}

