#pragma once

#include "FunctionWithManyArgs.h"
#include "Symbolic.h"

namespace MbD {

    class Product : public FunctionWithManyArgs
    {
    public:
        Product() : FunctionWithManyArgs() {}
        Product(Symsptr term) : FunctionWithManyArgs(term) {}
        Product(Symsptr term, Symsptr term1) : FunctionWithManyArgs(term, term1) {}
        Product(Symsptr term, Symsptr term1, Symsptr term2) : FunctionWithManyArgs(term, term1, term2) {}
        Product(std::shared_ptr<std::vector<Symsptr>> _terms) : FunctionWithManyArgs(_terms) {}
        Symsptr differentiateWRT(Symsptr sptr, Symsptr var) override;
        Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr timesSum(Symsptr sptr, Symsptr sum) override;
        Symsptr timesProduct(Symsptr sptr, Symsptr product) override;
        Symsptr timesFunction(Symsptr sptr, Symsptr function) override;
        std::ostream& printOn(std::ostream& s) const override;
        bool isProduct() override;
        double getValue() override;
    };
}

