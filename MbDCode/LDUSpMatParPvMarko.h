#pragma once

#include "LDUSpMatParPv.h"

namespace MbD {
    class LDUSpMatParPvMarko : public LDUSpMatParPv
    {
        //
    public:
        void doPivoting(int p) override;

    };
}

