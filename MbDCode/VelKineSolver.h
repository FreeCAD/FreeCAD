#pragma once

#include "VelSolver.h"

namespace MbD {
    class VelKineSolver : public VelSolver
    {
        //
    public:
        void assignEquationNumbers() override;
        void run() override;

    };
}

