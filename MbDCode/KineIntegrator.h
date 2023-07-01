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

