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
        void preStep();
        double suggestSmallerOrAcceptFirstStepSize(double hnew) override;
        void incrementTime(double tnew) override;
    };
}

