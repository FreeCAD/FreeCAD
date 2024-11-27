/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

#include "VelSolver.h"

namespace MbD {
    class VelICSolver : public VelSolver
    {
      //IC with fully or under constrained system
      //nqsu
    public:
        void assignEquationNumbers() override;
        void run() override;
        void runBasic();

        size_t nqsu = SIZE_MAX;
    };
}

