#pragma once

#include "AnyPosICNewtonRaphson.h"

namespace MbD {
    class PosICNewtonRaphson : public AnyPosICNewtonRaphson
    {
        //pivotRowLimits
    public:
        PosICNewtonRaphson(){}

        void run() override;
        void preRun() override;
        void assignEquationNumbers() override;
        bool isConverged() override;
        void handleSingularMatrix() override;
        void lookForRedundantConstraints();

        std::shared_ptr<std::vector<int>> pivotRowLimits;
    };
}

