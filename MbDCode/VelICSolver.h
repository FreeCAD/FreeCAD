#pragma once

#include "VelSolver.h"

namespace MbD {
    class VelICSolver : public VelSolver
    {
        //nqsu
    public:
        void assignEquationNumbers() override;
        void run() override;
        void runBasic();

        int nqsu;
    };
}

