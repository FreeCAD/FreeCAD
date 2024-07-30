/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "FunctionWithManyArgs.h"

namespace MbD {
    class Sum : public FunctionWithManyArgs
    {
    public:
        static Symsptr parseExpression(const std::string& expression);
        void parse(std::istringstream& iss);
        void parseTerm(std::istringstream& iss);
        void parsePlusTerm(std::istringstream& iss);
        void parseMinusTerm(std::istringstream& iss);

        Sum() : FunctionWithManyArgs() {}
        Sum(Symsptr term) : FunctionWithManyArgs(term) {}
        Sum(Symsptr term, Symsptr term1) : FunctionWithManyArgs(term, term1) {}
        Sum(Symsptr term, Symsptr term1, Symsptr term2) : FunctionWithManyArgs(term, term1, term2) {}
        Sum(std::shared_ptr<std::vector<Symsptr>> _terms) : FunctionWithManyArgs(_terms) {}
        Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        bool isSum() override;
        double getValue() override;
        Symsptr clonesptr() override;
        Symsptr differentiateWRT(Symsptr var) override;
        Symsptr integrateWRT(Symsptr var) override;

        std::ostream& printOn(std::ostream& s) const override;
    
    };
}

