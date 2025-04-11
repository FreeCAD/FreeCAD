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
    class FunctionXcParameter : public FunctionX
    {
        //
    public:
        FunctionXcParameter() = default;
        FunctionXcParameter(Symsptr arg);

    };
}

