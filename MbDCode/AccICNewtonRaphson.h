#pragma once

#include "AccNewtonRaphson.h"

namespace MbD {
    class AccICNewtonRaphson : public AccNewtonRaphson
    {
        //IC acceleration with fully or under constrained system
    public:
        bool isConverged() override;
        void preRun() override;


    };
}

