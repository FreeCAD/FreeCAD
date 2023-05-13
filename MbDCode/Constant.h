#pragma once

#include "Variable.h"

namespace MbD {
    class Constant : public Variable
    {
    public:
        Constant();
        Constant(double val);
        std::shared_ptr<Symbolic> differentiateWRT(std::shared_ptr<Symbolic> var) override;
    };
}

