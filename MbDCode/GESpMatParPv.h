#pragma once

#include "GESpMat3.h"

namespace MbD {
    class GESpMatParPv : public GESpMat
    {
        //
    public:
        void forwardEliminateWithPivot(size_t p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;

    };
}

