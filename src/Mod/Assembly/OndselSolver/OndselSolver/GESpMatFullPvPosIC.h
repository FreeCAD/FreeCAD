/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "GESpMatFullPv.h"

namespace MbD {
    class PosICNewtonRaphson;

    class GESpMatFullPvPosIC : public GESpMatFullPv
    {
        //system pivotRowLimits pivotRowLimit 
    public:
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void doPivoting(size_t p) override;

        PosICNewtonRaphson* system; //Use raw pointer when pointing backwards.
        std::shared_ptr<std::vector<size_t>> pivotRowLimits;
        size_t pivotRowLimit;
    };
}

