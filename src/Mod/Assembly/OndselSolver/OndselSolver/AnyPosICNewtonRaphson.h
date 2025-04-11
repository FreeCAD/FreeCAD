/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

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
        void assignEquationNumbers() override;
        bool isConverged() override;

        size_t nqsu = SIZE_MAX;
        FColDsptr qsuOld;
        DiagMatDsptr qsuWeights;
        size_t nSingularMatrixError = SIZE_MAX;
    };
}

