#pragma once

#include "MatrixSolver.h"
#include "SparseMatrix.h"

namespace MbD {
    class MatrixGaussElimination : public MatrixSolver
    {
        //
    public:
        virtual void forwardEliminateWithPivot(int p) = 0;
    };
}

