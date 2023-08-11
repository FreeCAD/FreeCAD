/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "QuasiIntegrator.h"

namespace MbD {
    class KineIntegrator : public QuasiIntegrator
    {
        //
    public:
        void preRun() override;
        void firstStep() override;
        void subsequentSteps() override;
        void nextStep() override;
        void runInitialConditionTypeSolution() override;
        void iStep(int i) override;
        void selectOrder() override;
    };
}

