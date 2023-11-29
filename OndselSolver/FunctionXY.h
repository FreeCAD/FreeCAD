/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Function.h"

namespace MbD {
    class Symbolic;
    //using Symsptr = Symsptr;

    class FunctionXY : public Function
    {
        //x y 
    public:
        FunctionXY();
        FunctionXY(Symsptr base, Symsptr exp);
        void arguments(Symsptr args) override;
        virtual Symsptr differentiateWRTx() = 0;
        virtual Symsptr differentiateWRTy() = 0;
        bool isConstant() override;

        Symsptr x, y;

    };
}

