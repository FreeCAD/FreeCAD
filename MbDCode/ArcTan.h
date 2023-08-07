#pragma once

#include "FunctionX.h"

namespace MbD {
    class ArcTan : public FunctionX
    {
        //
    public:
        ArcTan() = default;
        ArcTan(Symsptr arg);
        double getValue() override;

        std::ostream& printOn(std::ostream& s) const override;

    };
}

