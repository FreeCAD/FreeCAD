#pragma once

#include "QuasiIntegrator.h"

namespace MbD {
    class KineIntegrator : public QuasiIntegrator
    {
        //
    public:
        void preRun() override;
        void runInitialConditionTypeSolution() override;
    };
}

