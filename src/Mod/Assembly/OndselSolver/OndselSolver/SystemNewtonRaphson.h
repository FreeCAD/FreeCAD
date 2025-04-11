/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "VectorNewtonRaphson.h"
#include "SparseMatrix.h"

namespace MbD {
    //class SparseMatrix;

    class SystemNewtonRaphson : public VectorNewtonRaphson
    {
        //
    public:
        void initializeGlobally() override;
        virtual void assignEquationNumbers() override = 0;
        virtual void createVectorsAndMatrices();
        std::shared_ptr<MatrixSolver> matrixSolverClassNew() override;
        void calcdxNorm() override;
        void basicSolveEquations() override;
        void handleSingularMatrix() override;
        void outputSpreadsheet();

        SpMatDsptr pypx;
    };
}

