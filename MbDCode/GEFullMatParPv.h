#pragma once

#include "GEFullMat.h"

namespace MbD {
    class GEFullMatParPv : public GEFullMat
    {
        //
    public:
        void doPivoting(size_t p) override;
        void postSolve() override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
    };
}

