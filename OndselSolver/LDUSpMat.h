/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "MatrixLDU.h"
#include "SparseMatrix.h"

namespace MbD {
    class LDUSpMat : public MatrixLDU
    {
        //matrixL matrixD matrixU markowitzPivotRowCount markowitzPivotColCount privateIndicesOfNonZerosInPivotRow rowPositionsOfNonZerosInPivotColumn
    public:
        using MatrixSolver::basicSolvewithsaveOriginal;
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void decomposesaveOriginal(FMatDsptr fullMat, bool saveOriginal);
        void decomposesaveOriginal(SpMatDsptr spMat, bool saveOriginal);
        FColDsptr forAndBackSubsaveOriginal(FColDsptr fullCol, bool saveOriginal) override;
        double getmatrixArowimaxMagnitude(size_t i) override;
        void forwardSubstituteIntoL() override;
        void backSubstituteIntoDU() override;

        SpMatDsptr matrixA, matrixL, matrixU;
        DiagMatDsptr matrixD;
        size_t markowitzPivotRowCount, markowitzPivotColCount;
        std::shared_ptr<std::vector<size_t>> rowPositionsOfNonZerosInPivotColumn;
    };
}

