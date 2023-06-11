#pragma once

#include "IntegratorInterface.h"

namespace MbD {
    class QuasiIntegrator : public IntegratorInterface
    {
        //
    public:
        void preRun() override;
        void initialize() override;
        void run() override;
        void preFirstStep();
        double suggestSmallerOrAcceptFirstStepSize(double hnew) override;
    };
}

