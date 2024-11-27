/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <cstdint>

#include "MatrixGaussElimination.h"
#include "SparseMatrix.h"

namespace MbD {
    class GESpMat : public MatrixGaussElimination
    {
        //markowitzPivotRowCount markowitzPivotColCount privateIndicesOfNonZerosInPivotRow rowPositionsOfNonZerosInPivotColumn 
    public:
        FColDsptr solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        double getmatrixArowimaxMagnitude(size_t i) override;

        SpMatDsptr matrixA;
        size_t markowitzPivotRowCount = SIZE_MAX, markowitzPivotColCount = SIZE_MAX;
        std::shared_ptr<std::vector<size_t>> rowPositionsOfNonZerosInPivotColumn;
    };
}

