#pragma once

#include "MatrixGaussElimination.h"
#include "SparseMatrix.h"

namespace MbD {
    class GESpMat : public MatrixGaussElimination
    {
        //markowitzPivotRowCount markowitzPivotColCount privateIndicesOfNonZerosInPivotRow rowPositionsOfNonZerosInPivotColumn 
    public:
        FColDsptr solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal);

        std::shared_ptr<SparseMatrix<double>> matrixA;
        int markowitzPivotRowCount, markowitzPivotColCount;
        std::shared_ptr<std::vector<int>> privateIndicesOfNonZerosInPivotRow, rowPositionsOfNonZerosInPivotColumn;
    };
}

