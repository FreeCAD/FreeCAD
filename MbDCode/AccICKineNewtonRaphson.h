#pragma once

#include "AccNewtonRaphson.h"

namespace MbD {
    class AccICKineNewtonRaphson : public AccNewtonRaphson
    {
        //
    public:
        void initializeGlobally() override;
        void preRun() override;


    };
}

