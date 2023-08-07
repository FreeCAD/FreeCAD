#pragma once

#include "AnyPosICNewtonRaphson.h"

namespace MbD {
    class PosICKineNewtonRaphson : public AnyPosICNewtonRaphson
    {
        //Kinematics with under constrained system
    public:
        void initializeGlobally() override;
        void assignEquationNumbers() override;
        void preRun() override;

    };
}

