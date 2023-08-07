#pragma once

#include "FunctionX.h"

namespace MbD {
    class Abs : public FunctionX
    {
        //
    public:
        Abs() = default;
        Abs(Symsptr arg);
        double getValue() override;

        std::ostream& printOn(std::ostream& s) const override;

    };
}

