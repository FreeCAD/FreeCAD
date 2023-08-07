#pragma once

#include "FunctionX.h"

namespace MbD {
    class LogN : public FunctionX
    {
        //
    public:
        LogN() = default;
        LogN(Symsptr arg);
        double getValue() override;

        std::ostream& printOn(std::ostream& s) const override;


    };
}

