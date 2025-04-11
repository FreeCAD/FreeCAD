/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "GESpMat.h"

namespace MbD {
    class GESpMatFullPv : public GESpMat
    {
        //positionsOfOriginalCols rowPositionsOfNonZerosInColumns 
    public:
        void doPivoting(size_t p) override;
        void forwardEliminateWithPivot(size_t p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;

        std::shared_ptr<std::vector<size_t>> positionsOfOriginalCols;
        std::shared_ptr<std::vector<std::shared_ptr<std::vector<size_t>>>> rowPositionsOfNonZerosInColumns;
    };
}

