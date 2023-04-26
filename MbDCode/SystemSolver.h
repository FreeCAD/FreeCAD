#pragma once
#include <memory>

#include "Solver.h"
#include "System.h"
#include "NewtonRaphson.h"

namespace MbD {
    class System;
    class SystemSolver : public Solver
    {
        //system parts jointsMotions forcesTorques sensors variables icTypeSolver setsOfRedundantConstraints errorTolPosKine errorTolAccKine 
        //iterMaxPosKine iterMaxAccKine basicIntegrator tstartPasts tstart hmin hmax tend toutFirst hout direction corAbsTol corRelTol 
        //intAbsTol intRelTol iterMaxDyn orderMax translationLimit rotationLimit 
    public:
        SystemSolver(System& x) : system(x) {
        }
        std::shared_ptr<NewtonRaphson> icTypeSolver;
        System& system;
    };
}

