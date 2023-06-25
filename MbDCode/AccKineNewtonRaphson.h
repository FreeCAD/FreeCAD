#pragma once

#include "AccNewtonRaphson.h"

namespace MbD {
    class AccKineNewtonRaphson : public AccNewtonRaphson
    {
        //
    public:
        void initializeGlobally() override;
        void preRun() override;


    };
}

