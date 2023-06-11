#pragma once

#include "PosNewtonRaphson.h"

namespace MbD {
    class PosKineNewtonRaphson : public PosNewtonRaphson
    {
        //
    public:
        void initializeGlobally() override;
    };
}

