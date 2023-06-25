#pragma once

#include "AccNewtonRaphson.h"

namespace MbD {
    class AccICNewtonRaphson : public AccNewtonRaphson
    {
        //
    public:
        bool isConverged() override;
        void preRun() override;


    };
}

