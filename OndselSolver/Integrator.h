/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <memory>
#include "Solver.h"

namespace MbD {
    class SystemSolver;

    class Integrator : public Solver
    {
        //system direction 
    public:
        void setSystem(Solver* sys) override;
        virtual void firstStep() = 0;
        virtual void preFirstStep() = 0;
        virtual void postFirstStep() = 0;
        virtual void subsequentSteps() = 0;
        virtual void nextStep() = 0;
        virtual void preStep() = 0;
        virtual void postStep() = 0;
        virtual void runInitialConditionTypeSolution() = 0;
        virtual void iStep(size_t i) = 0;
        virtual void selectOrder() = 0;

        double direction = 1;
    };
}

