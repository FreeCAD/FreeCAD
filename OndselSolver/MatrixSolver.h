/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "Solver.h"
#include "RowTypeMatrix.h"
#include "FullMatrix.h"
#include "FullColumn.h"
#include "SparseMatrix.h"

namespace MbD {
    class MatrixSolver : public Solver
    {
        //m n matrixA answerX rightHandSideB rowOrder colOrder rowScalings pivotValues singularPivotTolerance millisecondsToRun
    public:
        MatrixSolver(){}
        virtual ~MatrixSolver() {}
        void initialize() override;
        void setSystem(Solver* sys) override;
        virtual FColDsptr solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr solvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr timedSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr timedSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal);

        virtual FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) = 0;

        virtual void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void doPivoting(size_t p) = 0;
        virtual void forwardEliminateWithPivot(size_t p) = 0;
        virtual void backSubstituteIntoDU() = 0;

        virtual void postSolve() = 0;
        virtual void findScalingsForRowRange(size_t begin, size_t end);
        virtual double getmatrixArowimaxMagnitude(size_t i) = 0;
        void throwSingularMatrixError(const std::string& chars);
        void throwSingularMatrixError(const std::string& chars, std::shared_ptr<FullColumn<size_t>> redunEqnNos);

        size_t m = 0, n = 0;
        FColDsptr answerX, rightHandSideB, rowScalings, pivotValues;
        std::shared_ptr<FullColumn<size_t>> rowOrder;
        std::shared_ptr<FullRow<size_t>> colOrder;
        double singularPivotTolerance = 0, millisecondsToRun = 0;
    };
}

