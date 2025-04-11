/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "Solver.h"
//#include "RowTypeMatrix.h"

namespace MbD {
    template<typename T>
    class FullColumn;
    //class RowTypeMatrix;
    class SystemSolver;

    class NewtonRaphson : public Solver
    {
        //system xold x dx dxNorm dxNorms dxTol y yNorm yNormOld yNorms yNormTol pypx iterNo iterMax nDivergence nBackTracking twoAlp lam 
    public:
        void initialize() override;
        void initializeLocally() override;
        void run() override;
        void setSystem(Solver* sys) override;
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
        void postRun() override;
        
        SystemSolver* system = nullptr; //Use raw pointer when pointing backwards.
        std::shared_ptr<std::vector<double>> dxNorms, yNorms;
        double dxNorm = 0.0, yNorm = 0.0, yNormOld = 0.0, yNormTol = 0.0, dxTol = 0.0, twoAlp = 0.0, lam = 0.0;
        size_t iterNo = SIZE_MAX, iterMax = SIZE_MAX, nDivergence = SIZE_MAX, nBackTracking = SIZE_MAX;
    };
}

