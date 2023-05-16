#pragma once

#include "Function.h"

namespace MbD {
    class FunctionWithManyArgs : public Function
    {
        //terms
    public:
        FunctionWithManyArgs(std::shared_ptr<Symbolic> term);
        FunctionWithManyArgs(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1);
        FunctionWithManyArgs(std::shared_ptr<Symbolic> term, std::shared_ptr<Symbolic> term1, std::shared_ptr<Symbolic> term2);
        FunctionWithManyArgs(std::shared_ptr<std::vector<std::shared_ptr<Symbolic>>> _terms);

        std::shared_ptr<std::vector<std::shared_ptr<Symbolic>>> terms;
    };
}

