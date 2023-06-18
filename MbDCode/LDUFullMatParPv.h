#pragma once

#include "LDUFullMat.h"

namespace MbD {
	class LDUFullMatParPv : public LDUFullMat
    {
        //
    public:
        void doPivoting(int p) override;

    };
}

