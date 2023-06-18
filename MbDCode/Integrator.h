#pragma once
#include <memory>
#include "Solver.h"

namespace MbD {
    class SystemSolver;

    class Integrator : public Solver
    {
        //system direction 
    public:
        void setSystem(Solver* sys) override;
        virtual void runInitialConditionTypeSolution() = 0;

        double direction = 1;
    };
}

