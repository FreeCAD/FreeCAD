/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Variable.h"

namespace MbD {
    class IndependentVariable : public Variable
    {
    public:
        IndependentVariable();
        Symsptr differentiateWRT(Symsptr var) override;
    };
}

