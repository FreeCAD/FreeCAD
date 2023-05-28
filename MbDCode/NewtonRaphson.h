#pragma once

#include <memory>
#include <vector>

#include "Solver.h"
//#include "FullColumn.h"
//#include "SystemSolver.h"

namespace MbD {
    template <typename T>
    class FullColumn;
    class SystemSolver;

    class NewtonRaphson : public Solver
    {
        //system xold x dx dxNorm dxNorms dxTol y yNorm yNormOld yNorms yNormTol pypx iterNo iterMax nDivergence nBackTracking twoAlp lam 
    public:
        void initialize();
        void initializeLocally() override;
        void run() override;
        void setSystem(SystemSolver* sys);

        SystemSolver* system;
        std::shared_ptr<FullColumn<double>> xold, x, dx, y;
        std::shared_ptr<std::vector<double>> dxNorms, yNorms;
        double dxNorm, yNorm, yNormOld, yNormTol, dxTol, twoAlp, lam;
        int iterNo, iterMax, nDivergence, nBackTracking;
    };
}

