#pragma once

#include "AccNewtonRaphson.h"

namespace MbD {
    class AccKineNewtonRaphson : public AccNewtonRaphson
    {
        //Kinematics with fully constrained system
    public:
        void initializeGlobally() override;
        void preRun() override;


    };
}

