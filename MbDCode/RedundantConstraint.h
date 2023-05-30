#pragma once

#include "Item.h"

namespace MbD {
    class RedundantConstraint : public Item
    {
        //
    public:
        std::shared_ptr<Constraint> constraint;
    };
}

