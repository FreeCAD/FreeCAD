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
        void setSystem(Solver* sys) override;
        virtual FColDsptr solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr solvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr timedSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr timedSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal);
        virtual FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void doPivoting(int p) = 0;
        virtual void forwardEliminateWithPivot(int p) = 0;
        virtual void backSubstituteIntoDU() = 0;

        virtual void postSolve() = 0;
        virtual void findScalingsForRowRange(int begin, int end);
        virtual double getmatrixArowimaxMagnitude(int i) = 0;
        void throwSingularMatrixError(const char* chars);
        void throwSingularMatrixError(const char* chars, std::shared_ptr<FullColumn<int>> redunEqnNos);

        int m = 0, n = 0;
        FColDsptr answerX, rightHandSideB, rowScalings, pivotValues;
        std::shared_ptr<FullColumn<int>> rowOrder;
        std::shared_ptr<FullRow<int>> colOrder;
        double singularPivotTolerance = 0, millisecondsToRun = 0;
    };
}

