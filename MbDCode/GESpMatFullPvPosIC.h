#pragma once

#include "GESpMatFullPv.h"

namespace MbD {
    class PosICNewtonRaphson;

    class GESpMatFullPvPosIC : public GESpMatFullPv
    {
        //system pivotRowLimits pivotRowLimit 
    public:
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void doPivoting(int p) override;

        PosICNewtonRaphson* system; //Use raw pointer when pointing backwards.
        std::shared_ptr<std::vector<int>> pivotRowLimits;
        int pivotRowLimit;
    };
}

