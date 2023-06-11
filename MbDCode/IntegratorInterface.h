#pragma once

#include "Integrator.h"
//#include "BasicQuasiIntegrator.h"

namespace MbD {
    class BasicQuasiIntegrator;

    class IntegratorInterface : public Integrator
    {
        //tout hout hmin hmax tstart tend integrator 
    public:

        void initializeGlobally() override;
        virtual void preRun() = 0;
        void setSystem(SystemSolver* sys);
        void logString(std::string& str) override;
        void run() override;
        int orderMax();
        virtual void preFirstStep() = 0;
        virtual double suggestSmallerOrAcceptFirstStepSize(double hnew) = 0;

        SystemSolver* system;
        double tout = 0, hout = 0, hmin = 0, hmax = 0, tstart = 0, tend = 0;
        std::shared_ptr<BasicQuasiIntegrator> integrator;
    };
}

