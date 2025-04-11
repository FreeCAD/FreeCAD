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
        FColDsptr basicSolvewithsaveOriginal(FMatDsptr aMatrix, FColDsptr aVector, bool saveOriginal) override;
        virtual void forwardSubstituteIntoLD();
        void postSolve() override;
        virtual void preSolvesaveOriginal(FMatDsptr aMatrix, bool saveOriginal);

    };
}

