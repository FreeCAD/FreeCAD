#pragma once

#include "MatrixSolver.h"
#include "SparseMatrix.h"

namespace MbD {
    class MatrixGaussElimination : public MatrixSolver
    {
        //
    public:
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        virtual void forwardEliminateWithPivot(int p) = 0;
        virtual void backSubstituteIntoDU() = 0;
    };
}

