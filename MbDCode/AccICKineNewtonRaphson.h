#pragma once

#include "AccICNewtonRaphson.h"

namespace MbD {
    class AccICKineNewtonRaphson : public AccICNewtonRaphson
    {
        //Kinematics with under constrained system
    public:
        void initializeGlobally() override;
        void preRun() override;


    };
}

