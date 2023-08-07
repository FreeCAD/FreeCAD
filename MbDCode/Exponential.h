#pragma once

#include "FunctionX.h"

namespace MbD {
    class Exponential : public FunctionX
    {
        //
    public:
        Exponential() = default;
        Exponential(Symsptr arg);
        double getValue() override;

        std::ostream& printOn(std::ostream& s) const override;


    };
}

