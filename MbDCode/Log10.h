#pragma once

#include "FunctionX.h"

namespace MbD {
    class Log10 : public FunctionX
    {
        //
    public:
        Log10() = default;
        Log10(Symsptr arg);
        double getValue() override;

        std::ostream& printOn(std::ostream& s) const override;


    };
}

