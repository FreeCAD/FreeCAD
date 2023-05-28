#pragma once

#include "AnyPosICNewtonRaphson.h"

namespace MbD {
    class PosICNewtonRaphson : public AnyPosICNewtonRaphson
    {
        //pivotRowLimits
    public:
        PosICNewtonRaphson(){}

        void run() override;

        std::shared_ptr<std::vector<int>> pivotRowLimits;
    };
}

