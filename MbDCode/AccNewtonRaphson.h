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
        void passRootToSystem();
        void postRun() override;
        void preRun() override;


    };
}

