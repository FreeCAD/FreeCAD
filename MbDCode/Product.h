#pragma once

#include "FunctionWithManyArgs.h"

namespace MbD {
    class Product : public FunctionWithManyArgs
    {
    public:
        Product(std::shared_ptr<Symbolic> term) : FunctionWithManyArgs(term) {}
        Product(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1) : FunctionWithManyArgs(term, term1) {}
        Product(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1, std::shared_ptr<Symbolic> term2) : FunctionWithManyArgs(term, term1, term2) {}
        Product(std::shared_ptr<std::vector<std::shared_ptr<Symbolic>>> _terms) : FunctionWithManyArgs(_terms) {}
        double getValue() override;
    };
}

