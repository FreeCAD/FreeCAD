/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "NewtonRaphson.h"

namespace MbD {
    class ScalarNewtonRaphson : public NewtonRaphson
    {
        //
    public:
        void initializeGlobally() override;
        void calcyNorm() override;
        void solveEquations() override;
        void updatexold() override;
        void calcdxNorm() override;
        void xEqualxoldPlusdx() override;


        double xold, x, dx, y;
        double pypx;

    };
}

