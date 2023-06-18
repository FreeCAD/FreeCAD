#pragma once

#include "MatrixDecomposition.h"

namespace MbD {
    class MatrixLDU : public MatrixDecomposition
    {
        //
    public:
        FColDsptr forAndBackSubsaveOriginal(FColDsptr fullCol, bool saveOriginal) override;

    };
}

