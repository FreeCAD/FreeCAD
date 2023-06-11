#pragma once

#include "GESpMat.h"

namespace MbD {
    class GESpMatFullPv : public GESpMat
    {
        //positionsOfOriginalCols rowPositionsOfNonZerosInColumns 
    public:
        void doPivoting(int p) override;
        void forwardEliminateWithPivot(int p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;

        std::shared_ptr<std::vector<int>> positionsOfOriginalCols;
        std::shared_ptr<std::vector<std::shared_ptr<std::vector<int>>>> rowPositionsOfNonZerosInColumns;
    };
}

