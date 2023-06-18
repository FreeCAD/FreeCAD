#pragma once

#include "MatrixSolver.h"

namespace MbD {
    class MatrixDecomposition : public MatrixSolver
    {
        //
    public:
        virtual FColDsptr forAndBackSubsaveOriginal(FColDsptr fullCol, bool saveOriginal) = 0;
        virtual void applyRowOrderOnRightHandSideB();
        virtual void forwardSubstituteIntoL() = 0;
        //virtual void backSubstituteIntoU();
        //virtual FColDsptr basicSolve(aMatrix); with : aVector saveOriginal : saveOriginal
        //virtual void forwardSubstituteIntoL();
        //virtual void forwardSubstituteIntoLD();
        //virtual void postSolve();
        //virtual void preSolve : aMatrix saveOriginal : saveOriginal

    };
}

