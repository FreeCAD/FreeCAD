#pragma once

#include "GEFullMat.h"

namespace MbD {
    class GEFullMatParPv : public GEFullMat
    {
        //
    public:
        void doPivoting(int p) override;
        void postSolve() override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
    };
}

