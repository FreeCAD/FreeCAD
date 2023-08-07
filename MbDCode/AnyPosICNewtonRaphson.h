#pragma once

#include "PosNewtonRaphson.h"
#include "DiagonalMatrix.h"

namespace MbD {

    class AnyPosICNewtonRaphson : public PosNewtonRaphson
    {
        //IC with fully or under constrained system
        //nqsu qsuOld qsuWeights nSingularMatrixError 
    public:
        void initialize() override;
        void initializeGlobally() override;
        void createVectorsAndMatrices() override;
        void fillY() override;
        void fillPyPx() override;
        void passRootToSystem() override;
        void assignEquationNumbers() = 0;

        int nqsu = -1;
        FColDsptr qsuOld;
        DiagMatDsptr qsuWeights;
        int nSingularMatrixError = -1;
    };
}

