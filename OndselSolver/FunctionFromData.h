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
    class FunctionFromData : public FunctionXcParameter
    {
        //xvalue xs ys 
    public:
        FunctionFromData() = default;
        FunctionFromData(Symsptr arg);

        double xvalue = std::numeric_limits<double>::min();
        std::shared_ptr<std::vector<double>> xs, ys;
    };
}

