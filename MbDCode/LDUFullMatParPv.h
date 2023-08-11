/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "LDUFullMat.h"

namespace MbD {
	class LDUFullMatParPv : public LDUFullMat
    {
        //
    public:
        void doPivoting(int p) override;

    };
}

