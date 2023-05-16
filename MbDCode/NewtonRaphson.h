#pragma once

#include <memory>

#include "Solver.h"

#include "FullColumn.h"

namespace MbD {
    class NewtonRaphson : public Solver
    {
        //system xold x dx dxNorm dxNorms dxTol y yNorm yNormOld yNorms yNormTol pypx iterNo iterMax nDivergence nBackTracking twoAlp lam 
    public:
        std::shared_ptr<FullColumn<double>> xold, x, dx, dxTol, y;
        double dxNorm, yNorm, yNormOld, yNormTol, twoAlp, lam;
    };
}

