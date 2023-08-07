#pragma once

#include "FunctionXY.h"

namespace MbD {
    class Power : public FunctionXY
    {
        //
    public:
        Power();
        Power(Symsptr base, Symsptr exp);
        Symsptr differentiateWRTx() override;
        Symsptr differentiateWRTy() override;

        Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        double getValue() override;

    };
}

