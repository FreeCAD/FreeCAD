#pragma once

#include "PosNewtonRaphson.h"
#include "DiagonalMatrix.h"

namespace MbD {

    class AnyPosICNewtonRaphson : public PosNewtonRaphson
    {
        //nqsu qsuOld qsuWeights nSingularMatrixError 
    public:
        void initialize() override;
        void initializeGlobally() override;
        void createVectorsAndMatrices() override;
        void fillY() override;
        void fillPyPx() override;
        void passRootToSystem() override;

        size_t nqsu = -1;
        std::shared_ptr<FullColumn<double>> qsuOld;
        std::shared_ptr<DiagonalMatrix<double>> qsuWeights;
        size_t nSingularMatrixError = -1;
    };
}

