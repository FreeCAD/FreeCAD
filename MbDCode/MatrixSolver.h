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
        void initialize() override;
        virtual FColDsptr solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr solvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr timedSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr timedSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void doPivoting(int p) = 0;
        virtual void postSolve() = 0;

        int m = 0, n = 0;
        FColDsptr answerX, rightHandSideB, rowScalings, pivotValues;
        std::shared_ptr<FullColumn<int>> rowOrder, colOrder;
        double singularPivotTolerance = 0, millisecondsToRun = 0;
    };
}

