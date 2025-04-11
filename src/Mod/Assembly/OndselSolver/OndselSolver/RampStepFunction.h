/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "PiecewiseFunction.h"

namespace MbD {
    class RampStepFunction : public PiecewiseFunction
    {
    public:
        RampStepFunction() = default;
        RampStepFunction(Symsptr var, std::shared_ptr<std::vector<double>> consts, std::shared_ptr<std::vector<double>> trans);
        void arguments(Symsptr args) override;
        void initFunctionsTransitions(Symsptr var, double x0, double y0, double x1, double y1);
        void initFunctionsTransitions(Symsptr var, double x0, double y0, double x1, double y1, Symsptr symx0, Symsptr symy0, Symsptr symx1, Symsptr symy1);

    };
}

