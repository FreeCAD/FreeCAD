#pragma once

#include "Variable.h"

namespace MbD {
    class IndependentVariable : public Variable
    {
    public:
        IndependentVariable();
        Symsptr differentiateWRT(Symsptr var) override;
    };
}

