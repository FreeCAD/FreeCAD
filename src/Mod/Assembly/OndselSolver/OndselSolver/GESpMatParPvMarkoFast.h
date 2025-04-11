/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "GESpMatParPvMarko.h"

namespace MbD {
    class GESpMatParPvMarkoFast : public GESpMatParPvMarko
    {
        //
    public:
        void preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal) override;
        void doPivoting(size_t p) override;

    };
}

