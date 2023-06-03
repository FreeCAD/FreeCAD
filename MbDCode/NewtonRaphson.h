#pragma once

#include <memory>
#include <vector>

#include "Solver.h"
//#include "RowTypeMatrix.h"

namespace MbD {
    template <typename T>
    class FullColumn;
    //class RowTypeMatrix;
    class SystemSolver;

    class NewtonRaphson : public Solver
    {
        //system xold x dx dxNorm dxNorms dxTol y yNorm yNormOld yNorms yNormTol pypx iterNo iterMax nDivergence nBackTracking twoAlp lam 
    public:
        void initialize();
        void initializeLocally() override;
        void run() override;
        void setSystem(SystemSolver* sys);
        void iterate();
        virtual void fillY() = 0;
        virtual void fillPyPx() = 0;
        virtual void calcyNorm() = 0;
        virtual void calcdxNorm() = 0;
        virtual void solveEquations() = 0;
        virtual void incrementIterNo();
        virtual void updatexold() = 0;
        virtual void xEqualxoldPlusdx() = 0;

        virtual bool isConverged();
        virtual void askSystemToUpdate();
        virtual void passRootToSystem() = 0;
        bool isConvergedToNumericalLimit();
        void calcDXNormImproveRootCalcYNorm();
        
        SystemSolver* system; //Use raw pointer when pointing backwards.
        std::shared_ptr<std::vector<double>> dxNorms, yNorms;
        double dxNorm, yNorm, yNormOld, yNormTol, dxTol, twoAlp, lam;
        size_t iterNo = -1, iterMax = -1, nDivergence = -1, nBackTracking = -1;
    };
}

