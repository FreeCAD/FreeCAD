#pragma once

#include "ForceTorqueItem.h"

namespace MbD {
    class ConstantGravity : public ForceTorqueItem
    {
        //
    public:
        void submitToSystem(std::shared_ptr<Item> self) override;

        FColDsptr gXYZ;
    };
}

