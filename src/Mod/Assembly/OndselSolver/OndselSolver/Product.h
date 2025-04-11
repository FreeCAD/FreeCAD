/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "FunctionWithManyArgs.h"
#include "Symbolic.h"
#include "System.h"
#include "Units.h"

namespace MbD {

    class Product : public FunctionWithManyArgs
    {
    public:
        Product() : FunctionWithManyArgs() {}
        Product(Symsptr term) : FunctionWithManyArgs(term) {}
        Product(Symsptr term, Symsptr term1) : FunctionWithManyArgs(term, term1) {}
        Product(Symsptr term, Symsptr term1, Symsptr term2) : FunctionWithManyArgs(term, term1, term2) {}
        Product(std::shared_ptr<std::vector<Symsptr>> _terms) : FunctionWithManyArgs(_terms) {}
        Symsptr differentiateWRT(Symsptr var) override;
        Symsptr integrateWRT(Symsptr var) override;
        Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        bool isProduct() override;
        double getValue() override;
        Symsptr clonesptr() override;

        std::ostream& printOn(std::ostream& s) const override;
    
    };
}

