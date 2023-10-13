/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "EigenDecomposition.h"

namespace MbD {
    class MomentOfInertiaSolver : public EigenDecomposition
    {
        //See document 9100moment.fodt
        //m aJPP aJoo rPoP aAPo aJcmP aJcmPcopy rPcmP aJpp aAPp colOrder 
        //aJoo == aJpp when rPoP == rPcmP and aAPo == aAPp
    public:
        static void example1();
        void forwardEliminateWithPivot(int p) override;
        void backSubstituteIntoDU() override;
        void postSolve() override;
        FColDsptr basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        FColDsptr basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        double getmatrixArowimaxMagnitude(int i) override;
        void doPivoting(int p) override;

        void setm(double mass);
        void setJPP(FMatDsptr mat);
        void setrPoP(FColDsptr col);
        void setAPo(FMatDsptr mat);
        void setrPcmP(FColDsptr col);
        FMatDsptr getJoo();
        FMatDsptr getJpp();
        FMatDsptr getAPp();
        void calc();
        void calcJoo();
        void calcJpp();
        void calcAPp();

        double m;
        FMatDsptr aJPP, aJoo, aAPo, aJcmP, aJcmPcopy, aJpp, aAPp;
        FColDsptr rPoP, rPcmP;
        std::shared_ptr<FullRow<int>> colOrder;

    };
}
