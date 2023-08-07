#pragma once

#include "FunctionX.h"

namespace MbD {
    class Cosine : public FunctionX
    {
        //
    public:
        Cosine() = default;
        Cosine(Symsptr arg);
        double getValue() override;
        Symsptr differentiateWRTx() override;

        std::ostream& printOn(std::ostream& s) const override;

    };
}

