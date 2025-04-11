/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "MatrixGaussElimination.h"

namespace MbD {
    class GEFullMat : public MatrixGaussElimination
    {
        //
    public:
        void forwardEliminateWithPivot(size_t p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;
        FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        double getmatrixArowimaxMagnitude(size_t i) override;

        FMatDsptr matrixA;
    };
}

