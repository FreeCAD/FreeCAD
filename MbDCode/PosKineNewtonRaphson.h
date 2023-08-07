#pragma once

#include "PosNewtonRaphson.h"

namespace MbD {
    class PosKineNewtonRaphson : public PosNewtonRaphson
    {
        //Kinematics with fully constrained system
    public:
        void initializeGlobally() override;
        void fillPyPx() override;
        void passRootToSystem() override;
        void assignEquationNumbers() override;
        void preRun() override;
        void fillY() override;

    };
}

