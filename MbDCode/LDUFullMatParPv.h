#pragma once

#include "LDUFullMat.h"

namespace MbD {
    class LDUFullMatParPv : public LDUFullMat
    {
        //
    public:
        FMatDsptr inversesaveOriginal(FMatDsptr fullMat, bool saveOriginal);

    };
}

