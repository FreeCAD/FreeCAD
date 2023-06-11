#pragma once

#include "GESpMatParPv.h"

namespace MbD {
    class GESpMatParPvMarko : public GESpMatParPv
    {
        //
    public:
        void doPivoting(int p) override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;

    };
}

