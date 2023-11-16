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
    class StepFunction : public PiecewiseFunction
    {
    public:
        StepFunction(Symsptr var, std::shared_ptr<std::vector<double>> consts, std::shared_ptr<std::vector<double>> trans);

    };
}

