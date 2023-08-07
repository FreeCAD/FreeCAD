#pragma once

#include "Symbolic.h"

namespace MbD {
    class Function : public Symbolic
    {
        //
    public:
        virtual void arguments(Symsptr args) = 0;

    };
}

