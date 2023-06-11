#pragma once

#include "LinearMultiStepMethod.h"

namespace MbD {
    class StableBackwardDifference : public LinearMultiStepMethod
    {
        //
    public:
        void formTaylorMatrix() override;
        void instanciateTaylorMatrix() override;
        void formTaylorRowwithTimeNodederivative(int i, int ii, int k) override;
    };
}

