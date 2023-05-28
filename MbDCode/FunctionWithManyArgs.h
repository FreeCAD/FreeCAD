#pragma once

#include "Function.h"
#include "Symbolic.h"

namespace MbD {

    class FunctionWithManyArgs : public Function
    {
        //terms
    public:
        FunctionWithManyArgs();
        FunctionWithManyArgs(Symsptr term);
        FunctionWithManyArgs(Symsptr term, Symsptr term1);
        FunctionWithManyArgs(Symsptr term, Symsptr term1, Symsptr term2);
        FunctionWithManyArgs(std::shared_ptr<std::vector<Symsptr>> _terms);
        std::shared_ptr<std::vector<Symsptr>> getTerms() override;

        std::shared_ptr<std::vector<Symsptr>> terms;
    };
}

