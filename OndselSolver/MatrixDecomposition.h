/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "MatrixSolver.h"

namespace MbD {
    class MatrixDecomposition : public MatrixSolver
    {
        //
    public:
        virtual FColDsptr forAndBackSubsaveOriginal(FColDsptr fullCol, bool saveOriginal);
        virtual void applyRowOrderOnRightHandSideB();
        virtual void forwardSubstituteIntoL();
        virtual void backSubstituteIntoU();
        virtual FColDsptr basicSolvewithsaveOriginal(FMatDsptr aMatrix, FColDsptr aVector, bool saveOriginal);
        virtual void forwardSubstituteIntoLD();
        virtual void postSolve();
        virtual void preSolvesaveOriginal(FMatDsptr aMatrix, bool saveOriginal);

    };
}

