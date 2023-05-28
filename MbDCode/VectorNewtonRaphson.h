#pragma once

#include "NewtonRaphson.h"
#include "MatrixSolver.h"

namespace MbD {
    class VectorNewtonRaphson : public NewtonRaphson
    {
        //matrixSolver n
    public:
        void run() override;
        virtual std::shared_ptr<MatrixSolver> matrixSolverClassNew();
        std::shared_ptr<MatrixSolver> matrixSolver;
        int n;
    };
}

