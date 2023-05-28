#pragma once

#include "SystemNewtonRaphson.h"

namespace MbD {
    class PosNewtonRaphson : public SystemNewtonRaphson
    {
        //
    public:
        void preRun() override;

    };
}

