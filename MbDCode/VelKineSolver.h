#pragma once

#include "VelSolver.h"

namespace MbD {
    class VelKineSolver : public VelSolver
    {
        //Kinematics with fully constrained system
    public:
        void assignEquationNumbers() override;
        void run() override;

    };
}

