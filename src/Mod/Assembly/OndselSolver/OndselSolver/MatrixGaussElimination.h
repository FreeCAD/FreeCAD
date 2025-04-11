/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "MatrixSolver.h"
#include "SparseMatrix.h"

namespace MbD {
    class MatrixGaussElimination : public MatrixSolver
    {
        //
    public:
        virtual void forwardEliminateWithPivot(size_t p) = 0;
    };
}

