#pragma once

#include "FunctionX.h"

namespace MbD {
    class Ln : public FunctionX
    {
        //
    public:
        Ln() = default;
        Ln(Symsptr arg);
        double getValue() override;



    };
}

