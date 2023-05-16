#pragma once

#include "FunctionWithManyArgs.h"

namespace MbD {
    class Sum : public FunctionWithManyArgs
    {
    public:
        Sum(std::shared_ptr<Symbolic> term) : FunctionWithManyArgs(term) {}
        Sum(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1) : FunctionWithManyArgs(term, term1) {}
        Sum(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1, std::shared_ptr<Symbolic> term2) : FunctionWithManyArgs(term, term1, term2) {}
        Sum(std::shared_ptr<std::vector<std::shared_ptr<Symbolic>>> _terms) : FunctionWithManyArgs(_terms) {}
        double getValue() override;
    };
}

