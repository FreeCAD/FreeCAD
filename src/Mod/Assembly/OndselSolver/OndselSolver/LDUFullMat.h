/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "MatrixLDU.h"

namespace MbD {
    class LDUFullMat : public MatrixLDU
    {
        //
    public:
        FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void forwardEliminateWithPivot(size_t p) override;
        
        void postSolve() override;
        void preSolvesaveOriginal(FMatDsptr fullMat, bool saveOriginal) override;
        void decomposesaveOriginal(SpMatDsptr spMat, bool saveOriginal);
        void decomposesaveOriginal(FMatDsptr fullMat, bool saveOriginal);
        FMatDsptr inversesaveOriginal(FMatDsptr fullMat, bool saveOriginal);
        double getmatrixArowimaxMagnitude(size_t i) override;
        void forwardSubstituteIntoL() override;
        void backSubstituteIntoDU() override;

        FMatDsptr matrixA;

    };
}

