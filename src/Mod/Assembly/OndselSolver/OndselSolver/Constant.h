/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Variable.h"

namespace MbD {
    class Constant : public Variable
    {
    public:
        Constant();
        Constant(double val);
        Symsptr differentiateWRT(Symsptr var) override;
        Symsptr integrateWRT(Symsptr var) override;
        Symsptr expandUntil(Symsptr sptr, std::shared_ptr<std::unordered_set<Symsptr>> set) override;
        bool isConstant() override;
        Symsptr clonesptr() override;
        bool isZero() override;
        bool isOne() override;
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        double getValue() override;
        double getValue(double arg) override;

        std::ostream& printOn(std::ostream& s) const override;
    };
}

