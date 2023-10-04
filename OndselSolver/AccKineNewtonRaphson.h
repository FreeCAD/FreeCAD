/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "AccNewtonRaphson.h"

namespace MbD {
    class AccKineNewtonRaphson : public AccNewtonRaphson
    {
        //Kinematics with fully constrained system
    public:
        void initializeGlobally() override;
        void preRun() override;


    };
}

