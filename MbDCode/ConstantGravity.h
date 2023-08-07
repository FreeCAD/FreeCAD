#pragma once

#include "ForceTorqueItem.h"

namespace MbD {
    class ConstantGravity : public ForceTorqueItem
    {
        //
    public:
        void fillAccICIterError(FColDsptr col) override;

        FColDsptr gXYZ;
    };
}

