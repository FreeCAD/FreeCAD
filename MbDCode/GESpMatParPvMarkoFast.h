#pragma once

#include "GESpMatParPvMarko.h"

namespace MbD {
    class GESpMatParPvMarkoFast : public GESpMatParPvMarko
    {
        //
    public:
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void doPivoting(size_t p) override;

    };
}

