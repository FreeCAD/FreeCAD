#pragma once

#include "Variable.h"

namespace MbD {
    class Constant : public Variable
    {
    public:
        Constant();
        Constant(double val);
    };
}

