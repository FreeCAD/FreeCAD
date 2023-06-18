#pragma once

#include "LDUSpMatParPv.h"

namespace MbD {
    class LDUSpMatParPvPrecise : public LDUSpMatParPv
    {
        //
    public:
        void doPivoting(int p) override;

    };
}

