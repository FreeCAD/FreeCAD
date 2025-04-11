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
    class Ln : public FunctionX
    {
        //
    public:
        Ln() = default;
        Ln(Symsptr arg);
        double getValue() override;
        Symsptr copyWith(Symsptr arg) override;



    };
}

