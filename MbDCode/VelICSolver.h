#pragma once

#include "VelSolver.h"

namespace MbD {
    class VelICSolver : public VelSolver
    {
      //IC with fully or under constrained system
      //nqsu
    public:
        void assignEquationNumbers() override;
        void run() override;
        void runBasic();

        int nqsu;
    };
}

