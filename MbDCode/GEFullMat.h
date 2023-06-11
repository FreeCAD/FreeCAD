#pragma once

#include "MatrixGaussElimination.h"

namespace MbD {
    class GEFullMat : public MatrixGaussElimination
    {
        //
    public:
        std::shared_ptr<FullMatrix<double>> matrixA;
        void forwardEliminateWithPivot(int p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
    };
}

