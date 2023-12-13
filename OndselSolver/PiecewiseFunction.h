/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "FunctionXcParameter.h"

namespace MbD {
    class Symbolic;
    using Symsptr = std::shared_ptr<Symbolic>;
    
    class PiecewiseFunction : public FunctionXcParameter
    {
        //functions transitions 
        //func0 tran0 func1 tran1 func2
    public:
        PiecewiseFunction();
        PiecewiseFunction(Symsptr var, std::shared_ptr<std::vector<Symsptr>> funcs, std::shared_ptr<std::vector<Symsptr>> trans);
        Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr differentiateWRTx() override;
        Symsptr integrateWRT(Symsptr var) override;
        double getValue() override;
        void arguments(Symsptr args) override;

        std::ostream& printOn(std::ostream& s) const override;

        std::shared_ptr<std::vector<Symsptr>> functions = std::make_shared<std::vector<Symsptr>>();
        std::shared_ptr<std::vector<Symsptr>> transitions = std::make_shared<std::vector<Symsptr>>();

    };
}
