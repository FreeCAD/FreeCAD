#pragma once

#include "GEFullMat.h"

namespace MbD {
    class GEFullMatFullPv : public GEFullMat
    {
        //
    public:
        void doPivoting(size_t p) override;
        void postSolve() override;
    };
}

