#pragma once

#include "PosNewtonRaphson.h"

namespace MbD {
    class AnyPosICNewtonRaphson : public PosNewtonRaphson
    {
        //nqsu qsuOld qsuWeights nSingularMatrixError 
    public:
        int nqsu;
        std::shared_ptr<FullColumn<double>> qsuOld, qsuWeights;
        int nSingularMatrixError;
    };
}

