#pragma once

#include "GESpMat.h"

namespace MbD {
    class GESpMatParPv : public GESpMat
    {
        //
    public:
        void forwardEliminateWithPivot(int p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;

    };
}

