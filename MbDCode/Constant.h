#pragma once

#include "Variable.h"

namespace MbD {
    class Constant : public Variable
    {
    public:
        Constant();
        Constant(double val);
        Symsptr differentiateWRT(Symsptr var) override;
        bool isConstant() override;
        Symsptr expandUntil(std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr clonesptr() override;
        bool isZero() override;
        bool isOne() override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        double getValue() override;

        std::ostream& printOn(std::ostream& s) const override;
    };
}

