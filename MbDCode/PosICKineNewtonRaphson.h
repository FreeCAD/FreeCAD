#pragma once

#include "AnyPosICNewtonRaphson.h"

namespace MbD {
    class PosICKineNewtonRaphson : public AnyPosICNewtonRaphson
    {
        //
    public:
        void initializeGlobally() override;
        void assignEquationNumbers() override;
        void preRun() override;

    };
}

