#pragma once

#include "GEFullMat.h"

namespace MbD {
    class GEFullMatFullPv : public GEFullMat
    {
        //
    public:
        void doPivoting(int p) override;
        void postSolve() override;
    };
}

