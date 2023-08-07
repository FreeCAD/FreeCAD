#pragma once

#include "MatrixGaussElimination.h"

namespace MbD {
    class GEFullMat : public MatrixGaussElimination
    {
        //
    public:
        void forwardEliminateWithPivot(int p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;
        FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        double getmatrixArowimaxMagnitude(int i) override;

        FMatDsptr matrixA;
    };
}

