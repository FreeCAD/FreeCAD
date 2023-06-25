#pragma once
#include <vector>

#include "Integrator.h"

namespace MbD {
    class IntegratorInterface;
    class DifferenceOperator;

    class BasicIntegrator : public Integrator
    {
        //istep iTry maxTry tpast t tnew h hnew order orderNew orderMax opBDF continue 
    public:
        virtual void calcOperatorMatrix();
        virtual void incrementTime();
        virtual void incrementTry();
        void initialize() override;
        void initializeGlobally() override;
        void initializeLocally() override;
        virtual void iStep(int i);
        virtual void postFirstStep();
        void postRun() override;
        virtual void postStep();
        virtual void preFirstStep();
        virtual void preRun() override;
        virtual void preStep();
        virtual void reportStats() override;
        void run() override;
        virtual void selectOrder();
        virtual void subsequentSteps();
        void setSystem(Solver* sys) override;
        void logString(std::string& str) override;
        virtual void firstStep();
        virtual void nextStep() = 0;
        
        virtual void setorder(int o);
        virtual void settnew(double t);
        virtual void sett(double t);
        void settime(double t);

        IntegratorInterface* system;
        int istep = 0, iTry = 0, maxTry = 0;
        std::shared_ptr<std::vector<double>> tpast;
        double t = 0, tnew = 0, h = 0, hnew = 0;
        int order = 0, orderNew = 0, orderMax = 0;
        std::shared_ptr<DifferenceOperator> opBDF;
        bool _continue = false;
    };
}

