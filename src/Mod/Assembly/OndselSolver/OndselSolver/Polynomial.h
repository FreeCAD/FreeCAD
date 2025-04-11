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
    class Polynomial : public FunctionXcParameter
    {
        //pn = a0*x^0 + a1*x^1 ... an*x^n
    public:
        Polynomial(Symsptr var, std::shared_ptr<std::vector<double>> coeffs);
        Polynomial(Symsptr var, std::shared_ptr<std::vector<Symsptr>> coeffs);
        Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr simplifyUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        Symsptr differentiateWRTx() override;
        Symsptr integrateWRT(Symsptr var) override;
        double getValue() override;
        void setIntegrationConstant(double integConstant) override;

        std::ostream& printOn(std::ostream& s) const override;

        std::shared_ptr<std::vector<Symsptr>> coeffs = std::make_shared<std::vector<Symsptr>>();

    };
}

