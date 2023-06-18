#pragma once

#include "MatrixLDU.h"

namespace MbD {
    class LDUFullMat : public MatrixLDU
    {
        //
    public:
        FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void forwardEliminateWithPivot(int p) override;
        
        void postSolve() override;
        void preSolvesaveOriginal(FMatDsptr fullMat, bool saveOriginal);
        void decomposesaveOriginal(SpMatDsptr spMat, bool saveOriginal);
        void decomposesaveOriginal(FMatDsptr fullMat, bool saveOriginal);
        FMatDsptr inversesaveOriginal(FMatDsptr fullMat, bool saveOriginal);
        double getmatrixArowimaxMagnitude(int i) override;
        void forwardSubstituteIntoL() override;
        void backSubstituteIntoDU() override;

        std::shared_ptr<FullMatrix<double>> matrixA;

    };
}

