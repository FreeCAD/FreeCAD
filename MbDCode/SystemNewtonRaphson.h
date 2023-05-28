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
        virtual void assignEquationNumbers() = 0;
        virtual void createVectorsAndMatrices();
        std::shared_ptr<SparseMatrix<double>> pypx;
    };
}

