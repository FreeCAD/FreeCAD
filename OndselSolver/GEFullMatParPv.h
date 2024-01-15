/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "GEFullMat.h"

namespace MbD {
    class GEFullMatParPv : public GEFullMat
    {
        //
    public:
        void doPivoting(size_t p) override;
        void postSolve() override;
        void preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal) override;
    };
}

