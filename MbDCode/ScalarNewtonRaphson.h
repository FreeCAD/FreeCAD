#pragma once

#include "NewtonRaphson.h"

namespace MbD {
    class ScalarNewtonRaphson : public NewtonRaphson
    {
        //
    public:
        void calcyNorm() override;
        void solveEquations() override;
        void updatexold() override;
        void calcdxNorm() override;
        void xEqualxoldPlusdx() override;


        double xold, x, dx, y;
        double pypx;

    };
}

