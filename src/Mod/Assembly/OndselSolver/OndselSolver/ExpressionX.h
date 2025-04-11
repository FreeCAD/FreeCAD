/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "FunctionX.h"

namespace MbD {
    class ExpressionX : public FunctionX
    {
        //
    public:

        void xexpression(Symsptr arg, Symsptr func);
        Symsptr differentiateWRTx() override;
        Symsptr differentiateWRT(Symsptr var) override;
        double getValue() override;

        Symsptr expression = std::make_shared<Symbolic>();
    };
}

