/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "Symbolic.h"

namespace MbD {
    class Function : public Symbolic
    {
        //
    public:
        virtual void arguments(Symsptr args) = 0;

    };
}

