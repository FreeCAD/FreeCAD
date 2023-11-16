/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Function.h"
#include "Symbolic.h"
#include "System.h"
#include "Units.h"

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
        void createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits) override;
        void arguments(Symsptr args) override;
        bool isConstant() override;

        std::shared_ptr<std::vector<Symsptr>> terms;
    };
}

