#pragma once

#include "Variable.h"

namespace MbD {
    class Constant : public Variable
    {
    public:
        Constant();
        Constant(double val);
        Symsptr differentiateWRT(Symsptr sptr, Symsptr var) override;
        bool isConstant() override;
        std::ostream& printOn(std::ostream& s) const override;
    };
}

