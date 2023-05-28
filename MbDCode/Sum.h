#pragma once

#include "FunctionWithManyArgs.h"

namespace MbD {
    class Sum : public FunctionWithManyArgs
    {
    public:
        Sum() : FunctionWithManyArgs() {}
        Sum(Symsptr term) : FunctionWithManyArgs(term) {}
        Sum(Symsptr term, Symsptr term1) : FunctionWithManyArgs(term, term1) {}
        Sum(Symsptr term, Symsptr term1, Symsptr term2) : FunctionWithManyArgs(term, term1, term2) {}
        Sum(std::shared_ptr<std::vector<Symsptr>> _terms) : FunctionWithManyArgs(_terms) {}
        Symsptr timesSum(Symsptr sptr, Symsptr sum) override;
        Symsptr timesProduct(Symsptr sptr, Symsptr product) override;
        Symsptr timesFunction(Symsptr sptr, Symsptr function) override;
        Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        bool isSum() override;
        double getValue() override;
        std::ostream& printOn(std::ostream& s) const override;
    };
}

