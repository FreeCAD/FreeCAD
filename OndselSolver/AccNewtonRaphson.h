/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "SystemNewtonRaphson.h"

namespace MbD {
    class AccNewtonRaphson : public SystemNewtonRaphson
    {
        //
    public:
        void askSystemToUpdate() override;
        void assignEquationNumbers() override;
        void fillPyPx() override;
        void fillY() override;
        void incrementIterNo() override;
        void initializeGlobally() override;
        void logSingularMatrixMessage();
        void passRootToSystem() override;
        void postRun() override;
        void preRun() override;
        void handleSingularMatrix() override;


    };
}

