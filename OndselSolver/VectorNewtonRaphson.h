/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "NewtonRaphson.h"
#include "MatrixSolver.h"

namespace MbD {
    class VectorNewtonRaphson : public NewtonRaphson
    {
        //matrixSolver n
    public:
        void initializeGlobally() override;
        void run() override;
        virtual std::shared_ptr<MatrixSolver> matrixSolverClassNew();
        void fillY() override;
        void calcyNorm() override;
        void solveEquations() override;
        void updatexold() override;
        void calcdxNorm() override;
        bool isConverged() override;
        void xEqualxoldPlusdx() override;
        virtual void basicSolveEquations() = 0;

        std::shared_ptr<MatrixSolver> matrixSolver;
        int n;
        FColDsptr xold, x, dx, y;
        //std::shared_ptr<RowTypeMatrix<double>> pypx;
    };
}

